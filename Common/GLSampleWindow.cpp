#include "GLSampleWindow.h"
#include "Common.h"
#include "Core/Library.h"
#include "Core/SystemInfo.h"
#include "FPSCounter.h"
#include "GLUIComponent.h"

#include "GraphicFormat.h"
#include "IO/FileSystem.h"
#include "PostProcessing/BloomPostProcessing.h"
#include "PostProcessing/BlurPostProcessing.h"
#include "PostProcessing/ChromaticAberrationPostProcessing.h"
#include "PostProcessing/ColorGradePostProcessing.h"
#include "PostProcessing/ColorSpaceConverter.h"
#include "PostProcessing/DepthOfFieldPostProcessing.h"
#include "PostProcessing/FXAAPostProcessing.h"
#include "PostProcessing/GrainPostProcessing.h"
#include "PostProcessing/MistPostProcessing.h"
#include "PostProcessing/PixelatePostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "PostProcessing/PostProcessingManager.h"
#include "PostProcessing/SSAOPostProcessing.h"
#include "PostProcessing/SSSPostProcessing.h"
#include "PostProcessing/SobelPostProcessing.h"

#include "PostProcessing/VignetteProcessing.h"
#include "PostProcessing/VolumetricScattering.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "SampleHelper.h"
#include "imgui.h"
#include "magic_enum.hpp"
#include "spdlog/common.h"
#include "spdlog/logger.h"
#include <GL/glew.h>
#include <GLRendererInterface.h>
#include <ImageLoader.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <renderdoc_app.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>

using namespace glsample;

static unsigned int pboBuffer;

class SampleSettingComponent : public GLUIComponent<GLSampleWindow> {
  public:
	SampleSettingComponent(GLSampleWindow &base) : GLUIComponent(base) {}

	void draw() override {

		if (ImGui::CollapsingHeader("Debug Status")) {
			ImGui::SeparatorText("Debug");
			bool isDebug = this->getRefSample().isDebug();
			if (ImGui::Checkbox("Debug", &isDebug)) {
				this->getRefSample().debug(isDebug);
			}

			ImGui::BeginGroup();
			ImGui::Text("FPS %d", this->getRefSample().getFPSCounter().getFPS());
			ImGui::Text("FrameCount %zu", this->getRefSample().getFrameCount());
			ImGui::Text("Frame Index %zu", this->getRefSample().getFrameBufferIndex());

			ImGui::Text("Elapsed Time %.6f ms", (float)this->getRefSample().debugInfo.time_elapsed /
													(float)this->getRefSample().debugInfo.time_resolution);
			ImGui::Text("Primitive %zu", this->getRefSample().debugInfo.debug_prev_frame_primitive_count);
			ImGui::Text("Samples %zu", this->getRefSample().debugInfo.debug_prev_frame_sample_count);
			ImGui::Text("CS invocation %zu", this->getRefSample().debugInfo.debug_prev_frame_cs_invocation_count);
			ImGui::Text("Frag invocation %zu", this->getRefSample().debugInfo.debug_prev_frame_frag_invocation_count);
			ImGui::Text("Vertex invocation %zu",
						this->getRefSample().debugInfo.debug_prev_frame_vertex_invocation_count);
			ImGui::Text("Geometry invocation %zu",
						this->getRefSample().debugInfo.debug_prev_frame_geometry_invocation_count);

			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginGroup();
			bool renderDocEnable = this->getRefSample().isRenderDocEnabled();
			if (ImGui::Checkbox("RenderDoc", &renderDocEnable)) {
				this->getRefSample().enableRenderDoc(renderDocEnable);
			}
			if (ImGui::Button("Launch RenderDoc")) {
				this->getRefSample().launchRenderDoc();
			}
			/*	*/
			if (ImGui::Button("Capture Frame")) {
				this->getRefSample().captureDebugFrame();
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			ImGui::BeginGroup();

			ImGui::BeginDisabled(this->getRefSample().getDefaultFramebuffer() == 0);
			if (this->getRefSample().getDefaultFramebuffer() > 0) {
				GLboolean isMSAAEnabled = 0;
				glGetBooleanv(GL_MULTISAMPLE, &isMSAAEnabled);
				if (ImGui::Checkbox("MultiSampling (MSAA)", (bool *)&isMSAAEnabled)) {

					if (isMSAAEnabled) {
						glEnable(GL_MULTISAMPLE);
					} else {
						glDisable(GL_MULTISAMPLE);
					}
				}
				// TODO: add support to set.
				// if (ImGui::DragInt("MSAA Samples", &this->getRefSample().MSAA, 1.0f, 1, 8)) {
				//	/*	Resize the framebuffer required.	*/
				//	this->getRefSample().updateDefaultFramebuffer();
				// }

				bool isSuperEnabled = this->getRefSample().useSSAA;
				if (ImGui::Checkbox("SuperSampling Anti-Aliasing (SSAA)", (&isSuperEnabled))) {
					this->getRefSample().useSSAA = isSuperEnabled;
				}
				if (ImGui::DragInt("SSAA Samples", &this->getRefSample().SSAASamples, 1.0f, 1, 8)) {
					/*	Resize the framebuffer required.	*/
					this->getRefSample().updateDefaultFramebuffer();
				}

				bool useSampleAccum = this->getRefSample().useSampleAccumlation;
				if (ImGui::Checkbox("Use Sample Accumlation", (&useSampleAccum))) {
					this->getRefSample().useSampleAccumlation = useSampleAccum;
				}

				float min_sample = 0;
				glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE, &min_sample);
				if (ImGui::DragFloat("Min Sample", &min_sample, 1, 0, 1)) {
					glEnable(GL_SAMPLE_SHADING);
					glMinSampleShading(min_sample);
				}

				ImGui::Text("FrameBuffer Size: %zu:%zu", this->getRefSample().getCurrentFrameBufferWidth(),
							this->getRefSample().getCurrentFrameBufferHeight());
			}
			ImGui::EndDisabled();

			bool isVsync = this->getRefSample().getVSync();
			if (ImGui::Checkbox("VSync", &isVsync)) {
				this->getRefSample().vsync(isVsync);
			}

			ImGui::EndGroup();
		}

		ImGui::Text("WorkDirectory: %s", fragcore::SystemInfo::getCurrentDirectory().c_str());

		{
			const int item_selected_idx = (int)this->getRefSample().getLogger().level();

			std::string combo_preview_value =
				std::string(magic_enum::enum_name(this->getRefSample().getLogger().level()));
			ImGuiComboFlags flags = 0;
			if (ImGui::BeginCombo("Logging Level", combo_preview_value.c_str(), flags)) {
				for (int n = 0; n < (int)spdlog::level::level_enum::n_levels; n++) {
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(magic_enum::enum_name((spdlog::level::level_enum)n).data(), is_selected)) {
						this->getRefSample().getLogger().set_level((spdlog::level::level_enum)n);
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}

		/*	*/
		ImGui::BeginDisabled(this->getRefSample().getDefaultFramebuffer() == 0 ||
							 this->getRefSample().getColorSpaceConverter() == nullptr);
		ImGui::SeparatorText("Color Space Settings");
		if (this->getRefSample().getColorSpaceConverter()) {
			const int item_selected_idx = (int)this->getRefSample().getColorSpace();

			std::string combo_preview_value = std::string(magic_enum::enum_name(this->getRefSample().getColorSpace()));
			ImGuiComboFlags flags = 0;
			if (ImGui::BeginCombo("ColorSpace", combo_preview_value.c_str(), flags)) {
				for (int n = 0; n < (int)ColorSpace::MaxColorSpaces; n++) {
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(magic_enum::enum_name((ColorSpace)n).data(), is_selected)) {
						this->getRefSample().setColorSpace((ColorSpace)n);
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::BeginGroup();
			ImGui::TextUnformatted("Gamma Correction Settings");
			ImGui::DragFloat("Exposure", &this->getRefSample().getColorSpaceConverter()->getGammeSettings().exposure);
			ImGui::DragFloat("Gamma", &this->getRefSample().getColorSpaceConverter()->getGammeSettings().gamma);
			ImGui::EndGroup();
		}
		ImGui::EndDisabled();

		/*	List all builtin post processing.	*/
		bool usePostProcessing = this->getRefSample().getIsPostProcessingEnabled();
		if (ImGui::Checkbox("Use Post Processing", &usePostProcessing)) {
			this->getRefSample().setPostProcessingEnabled(usePostProcessing);
		}

		if (this->getRefSample().getPostProcessingManager() && ImGui::CollapsingHeader("Post Processing")) {

			ImGui::BeginGroup();
			PostProcessingManager *manager = this->getRefSample().getPostProcessingManager();

			/*	*/
			for (size_t post_index = 0; post_index < manager->getNrPostProcessing(); post_index++) {
				PostProcessing &postEffect = manager->getPostProcessing(post_index);

				ImGui::PushID(post_index);
				ImGui::BeginDisabled(!postEffect.isSupported());

				/*	*/
				ImGui::TextUnformatted(postEffect.getName().c_str());

				ImGui::BeginGroup();
				if (ImGui::Button("Up")) {
					manager->swapPostProcessing(post_index, post_index - 1);
				}
				if (ImGui::Button("Down")) {
					manager->swapPostProcessing(post_index, post_index + 1);
				}
				ImGui::EndGroup();

				ImGui::SameLine();

				ImGui::BeginGroup();
				bool isEnabled = manager->isEnabled(post_index);
				if (ImGui::Checkbox("Enabled", &isEnabled)) {
					manager->enablePostProcessing(post_index, isEnabled);
				}
				ImGui::SetItemTooltip("Used or Not");
				float enabled_intensity = postEffect.getIntensity();
				if (ImGui::SliderFloat("Intensity", &enabled_intensity, 0.0, 1)) {
					postEffect.setIntensity(enabled_intensity);
				}

				{
					ImGui::PushID(post_index + 100);
					postEffect.renderUI();
					ImGui::PopID();
				}

				ImGui::Separator();

				ImGui::EndDisabled();

				ImGui::EndGroup();

				ImGui::PopID();
			}

			ImGui::EndGroup();
		}

		/*	Display All Framebuffer textures.	*/
		const glsample::FrameBuffer *framebuffer = this->getRefSample().getDefaultFrameBufferObj();
		if (ImGui::CollapsingHeader("FrameBuffer Texture Targets") && framebuffer) {
			/*	*/
			for (size_t attach_index = 0; attach_index < framebuffer->nrAttachments; attach_index++) {
				ImGui::BeginGroup();
				ImGui::Text("Attachment %zu", attach_index);
				ImGui::Image(static_cast<ImTextureID>(framebuffer->attachments[attach_index]), ImVec2(256, 256),
							 ImVec2(1, 1), ImVec2(0, 0));
				ImGui::EndGroup();

				ImGui::SameLine();
			}

			ImGui::BeginGroup();
			ImGui::TextUnformatted("Depth/Stencil");
			ImGui::Image(static_cast<ImTextureID>(framebuffer->attachments[framebuffer->depthIndex]), ImVec2(256, 256),
						 ImVec2(1, 1), ImVec2(0, 0));
			ImGui::EndGroup();
		}
	}

  private:
};

GLSampleWindow::GLSampleWindow() : preWidth(this->width()), preHeight(this->height()) {

	/* Create logger	*/
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_level(spdlog::level::trace);
	stdout_sink->set_color_mode(spdlog::color_mode::always);
	stdout_sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
	stdout_sink->set_pattern("%g:%# [%^%l%$] %v");

	/*	*/
	this->logger = std::shared_ptr<spdlog::logger>(new spdlog::logger("glsample", {stdout_sink}));
	this->logger->set_level(spdlog::level::trace);

	/*	*/
	this->enableDocking(false);

	/*	*/
	this->fpsCounter = FPSCounter<float>(60, this->getTimer().getTimeResolution());
	this->getTimer().start();

	/*	*/
	this->getRenderInterface()->setDebug(true);

	const int screen_grab_width_size = this->width();
	const int screen_grab_height_size = this->height();

	/*	*/
	glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(screen_grab_width_size * screen_grab_height_size * 4),
				 nullptr, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	*/
	glGenQueries(this->queries.size(), this->queries.data());
	int time_precision = 0;
	glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &time_precision);

	/*	Disable automatic framebuffer gamma correction, each application handle it manually.	*/
	glDisable(GL_FRAMEBUFFER_SRGB);

	/*	Disable multi sampling by default.	*/
	glDisable(GL_MULTISAMPLE);

	/*	*/
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	/*	*/
	glMinSampleShading(1);
	glEnable(GL_SAMPLE_SHADING);

	/*	Set Compile threads.	*/
	if (glMaxShaderCompilerThreadsKHR) {
		glMaxShaderCompilerThreadsKHR(Math::max<size_t>(1, fragcore::SystemInfo::getCPUCoreCount() / 2));
	}

	std::shared_ptr<SampleSettingComponent> settingComponent = std::make_shared<SampleSettingComponent>(*this);
	this->addUIComponent(settingComponent);
}

GLSampleWindow::~GLSampleWindow() = default;

void GLSampleWindow::internalInit() {

	const bool use_color_space = !this->getResult()["disable-colorspace"].as<bool>();
	const bool use_post_process = this->getResult()["use-postprocessing"].as<bool>();
	const size_t multi_sample_count = this->getResult()["multi-sample"].as<int>();
	const bool useFBO = multi_sample_count > 0 || use_post_process || use_color_space || true; // TODO: fix conditions.

	if (this->colorSpace == nullptr) {

		if (use_color_space) {
			this->colorSpace = std::make_shared<ColorSpaceConverter>();
			this->colorSpace->initialize(this->getFileSystem());
		}

		if (use_post_process) {
			this->postprocessingManager = std::make_shared<PostProcessingManager>(*this);

			std::shared_ptr<FXAAPostProcessing> fxaa = std::make_shared<FXAAPostProcessing>();
			fxaa->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(fxaa);

			std::shared_ptr<SSAOPostProcessing> ssao = std::make_shared<SSAOPostProcessing>();
			ssao->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(ssao);

			std::shared_ptr<SSSPostProcessing> sss = std::make_shared<SSSPostProcessing>();
			sss->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(sss);

			std::shared_ptr<SobelProcessing> sobelPostProcessing = std::make_shared<SobelProcessing>();
			sobelPostProcessing->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(sobelPostProcessing);

			std::shared_ptr<ColorGradePostProcessing> colorgrade = std::make_shared<ColorGradePostProcessing>();
			colorgrade->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(colorgrade);

			std::shared_ptr<PixelatePostProcessing> pixelate = std::make_shared<PixelatePostProcessing>();
			pixelate->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(pixelate);

			std::shared_ptr<GrainPostProcessing> grain = std::make_shared<GrainPostProcessing>();
			grain->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(grain);

			std::shared_ptr<DepthOfFieldProcessing> depthOfField = std::make_shared<DepthOfFieldProcessing>();
			depthOfField->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(depthOfField);

			std::shared_ptr<MistPostProcessing> mistFog = std::make_shared<MistPostProcessing>();
			mistFog->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(mistFog);

			std::shared_ptr<VolumetricScatteringPostProcessing> volumetric =
				std::make_shared<VolumetricScatteringPostProcessing>();
			volumetric->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(volumetric);

			std::shared_ptr<BlurPostProcessing> blur = std::make_shared<BlurPostProcessing>();
			blur->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(blur);

			std::shared_ptr<BloomPostProcessing> bloom = std::make_shared<BloomPostProcessing>();
			bloom->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(bloom);

			std::shared_ptr<ChromaticAberrationPostProcessing> chromatic =
				std::make_shared<ChromaticAberrationPostProcessing>();
			chromatic->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(chromatic);

			std::shared_ptr<VignetteProcessing> vignette = std::make_shared<VignetteProcessing>();
			vignette->initialize(this->getFileSystem());
			this->postprocessingManager->addPostProcessing(vignette);
		}
	}

	/*	Multi Sampling.	*/
	if (this->MMSAFrameBuffer == nullptr && multi_sample_count > 0) {

		this->MMSAFrameBuffer = std::make_shared<glsample::FrameBuffer>();
		memset(MMSAFrameBuffer.get(), 0, sizeof(*this->MMSAFrameBuffer));
		CommonUtil::createFrameBuffer(MMSAFrameBuffer.get(), 1);
	}

	/*	Framebuffer.	*/
	if (this->defaultFramebuffer == nullptr && useFBO) {
		this->defaultFramebuffer = std::make_shared<glsample::FrameBuffer>();
		memset(defaultFramebuffer.get(), 0, sizeof(*this->defaultFramebuffer));
		CommonUtil::createFrameBuffer(defaultFramebuffer.get(), 3);
	}

	/*	Update if not internal default framebuffer	*/
	if (getDefaultFramebuffer() > 0) {
		this->updateDefaultFramebuffer();
	}
}

void GLSampleWindow::displayMenuBar() {}

void GLSampleWindow::renderUI() {

	/*	Make sure all commands are flush before resizing.	*/
	if (this->preWidth != this->width() || this->preHeight != this->height()) {

		/*	Finish all commands before starting resizing buffers and etc.	*/
		glFinish();

		this->onResize(this->width(), this->height());

		this->updateDefaultFramebuffer();
	}

	/*	*/
	this->preWidth = this->width();
	this->preHeight = this->height();

	/*	Main Update function.	*/
	this->getInput().update();
	this->update();
	this->postprocessingManager->update(this->getTimer().deltaTime<float>());

	/*	*/
	if (this->debugGL) {
		glBeginQuery(GL_TIME_ELAPSED, this->queries[0]);
		glBeginQuery(GL_SAMPLES_PASSED, this->queries[1]);
		glBeginQuery(GL_PRIMITIVES_GENERATED, this->queries[2]);
		glBeginQuery(GL_COMPUTE_SHADER_INVOCATIONS_ARB, this->queries[3]);
		glBeginQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB, this->queries[4]);
		glBeginQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB, this->queries[5]);
		glBeginQuery(GL_GEOMETRY_SHADER_INVOCATIONS, this->queries[6]);
	}

	{

		/*	*/
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());

		/*	Default state before any draw call.	*/
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		/*	Main Draw Callback.	*/
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, sizeof("Draw"), "Draw");
		this->draw();
		glPopDebugGroup();

		/*	Transfer Multisampled texture to FBO.	*/
		if (this->MMSAFrameBuffer && this->MMSAFrameBuffer->framebuffer == this->getDefaultFramebuffer()) {
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, sizeof("MultiSampling to FBO"), "MultiSampling to FBO");

			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->MMSAFrameBuffer->framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);

			const size_t source_framebuffer_width = this->MMSAFrameBuffer->attachmentSize[0].x;
			const size_t source_framebuffer_height = this->MMSAFrameBuffer->attachmentSize[0].y;

			const size_t target_framebuffer_width = this->defaultFramebuffer->attachmentSize[0].x;
			const size_t target_framebuffer_height = this->defaultFramebuffer->attachmentSize[0].y;

			/*	*/
			glBlitFramebuffer(0, 0, source_framebuffer_width, source_framebuffer_height, 0, 0, target_framebuffer_width,
							  target_framebuffer_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			/*	Filter Nearest since image sizes are the same.	*/
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glPopDebugGroup();
		}

		/*	*/
		if (this->postprocessingManager && this->postProcessingEnabled) {

			const std::string postStage = "Post Processing";
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, postStage.size(), postStage.data());
			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			glViewport(0, 0, this->getCurrentFrameBufferWidth(), this->getCurrentFrameBufferHeight());
			this->postprocessingManager->render(
				this->defaultFramebuffer.get(),
				/*	Setup References.	*/
				{std::make_tuple<const GBuffer, unsigned int>(GBuffer::Albedo, 0u),
				 std::make_tuple<const GBuffer, unsigned int>(GBuffer::Depth,
															  (unsigned int)this->defaultFramebuffer->depthIndex),
				 std::make_tuple<const GBuffer, unsigned int>(GBuffer::IntermediateTarget, 1u),
				 std::make_tuple<const GBuffer, unsigned int>(GBuffer::IntermediateTarget2, 2u)});
			glPopDebugGroup();
		}

		/*	Transfer last result to the default OpenGL Framebuffer.	*/
		if (this->defaultFramebuffer) {
			if (this->getColorSpaceConverter() && colorSpace->isSupported()) {
				const std::string ColorSpaceConverterStage = "Color Space Conversion";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, ColorSpaceConverterStage.length(),
								 ColorSpaceConverterStage.c_str());
				glViewport(0, 0, this->getCurrentFrameBufferWidth(), this->getCurrentFrameBufferHeight());
				this->colorSpace->render(this->defaultFramebuffer->attachments[0]);
				glPopDebugGroup();
			}

			/*TODO:	Loop for each SSAA scale, to make use of interpolation correctly.	*/

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);

			const size_t framebuffer_width = this->defaultFramebuffer->attachmentSize[0].x;
			const size_t framebuffer_height = this->defaultFramebuffer->attachmentSize[0].y;

			glBlitFramebuffer(0, 0, framebuffer_width, framebuffer_height, 0, 0, this->width(), this->height(),
							  GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, sizeof("Post Draw"), "Post Draw");
	glViewport(0, 0, this->width(), this->height());
	this->postDraw();
	glPopDebugGroup();

	/*	Extract debugging information.	*/
	if (this->debugGL) {

		// glGetQueryObjectiv(queries[N-1], GL_QUERY_RESULT_AVAILABLE, &available);
		glEndQuery(GL_TIME_ELAPSED);
		glEndQuery(GL_SAMPLES_PASSED);
		glEndQuery(GL_PRIMITIVES_GENERATED);
		glEndQuery(GL_COMPUTE_SHADER_INVOCATIONS_ARB);
		glEndQuery(GL_FRAGMENT_SHADER_INVOCATIONS_ARB);
		glEndQuery(GL_VERTEX_SHADER_INVOCATIONS_ARB);
		glEndQuery(GL_GEOMETRY_SHADER_INVOCATIONS);

		//	glGetQueryObjectui64v
		if (glGetQueryObjectui64v) {
			glGetQueryObjectui64v(this->queries[0], GL_QUERY_RESULT, &debugInfo.time_elapsed);
			glGetQueryObjectui64v(this->queries[1], GL_QUERY_RESULT, &debugInfo.nrSamples);
			glGetQueryObjectui64v(this->queries[2], GL_QUERY_RESULT, &debugInfo.nrPrimitives);
			glGetQueryObjectui64v(this->queries[3], GL_QUERY_RESULT,
								  &this->debugInfo.debug_prev_frame_cs_invocation_count);
			glGetQueryObjectui64v(this->queries[4], GL_QUERY_RESULT,
								  &this->debugInfo.debug_prev_frame_frag_invocation_count);
			glGetQueryObjectui64v(this->queries[5], GL_QUERY_RESULT,
								  &this->debugInfo.debug_prev_frame_vertex_invocation_count);
			glGetQueryObjectui64v(this->queries[6], GL_QUERY_RESULT,
								  &this->debugInfo.debug_prev_frame_geometry_invocation_count);
		}

		this->debugInfo.debug_prev_frame_sample_count = debugInfo.nrSamples;
		this->debugInfo.debug_prev_frame_primitive_count = debugInfo.nrPrimitives;

		this->getLogger().debug("Samples: {} Primitives: {} Elapsed: {} ms", this->debugInfo.nrSamples,
								this->debugInfo.nrPrimitives,
								(float)this->debugInfo.time_elapsed / (float)this->debugInfo.time_resolution);
	}

	/*	*/
	this->frameCount++;
	this->frameBufferIndex = (this->frameBufferIndex + 1) % this->getFrameBufferCount();

	{
		/*	Check if screenshot button pressed.	*/
		if (this->getInput().getKeyPressed(SDL_SCANCODE_F12)) {
			this->captureScreenShot();
		}

		/*	Enter fullscreen via short command.	*/
		if (this->getInput().getKeyReleased(SDL_SCANCODE_RETURN) &&
			(this->getInput().getKeyPressed(SDL_SCANCODE_LCTRL) ||
			 this->getInput().getKeyPressed(SDL_SCANCODE_RCTRL))) {
			this->setFullScreen(!this->isFullScreen());
		}

		/*	*/
		if (this->getInput().getKeyReleased(SDL_SCANCODE_F1)) {
			this->enableImGUI(!this->isEnabled());
		}

		if (this->getInput().getKeyReleased(SDL_SCANCODE_F9)) {
			this->captureDebugFrame();
		}
		if (this->getInput().getKeyReleased(SDL_SCANCODE_F1)) {
			// this->setStatusBar(true);
		}
	}

	this->getFPSCounter().update(this->getTimer().getElapsed<float>());

	/*	*/
	this->getLogger().trace("FPS: {} Elapsed Time: {} ({} ms)", this->getFPSCounter().getFPS(),
							this->getTimer().getElapsed<float>(), this->getTimer().deltaTime<float>() * 1000);
	this->getTimer().update();
}

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + this->getRenderInterface()->getAPIVersion() +
								   " GLSL: "); // + std::to_string(this->getShaderVersion())
}

bool GLSampleWindow::isDebug() const noexcept {
	const fragcore::GLRendererInterface *interface = this->getGLRenderInterface();

	return this->debugGL;
}

void GLSampleWindow::debug(const bool enable) {

	fragcore::GLRendererInterface *interface = this->getGLRenderInterface();
	interface->setDebug(enable);
	this->debugGL = enable;

	if (enable) {
		this->logger->set_level(spdlog::level::trace);
	} else {
		this->logger->set_level(spdlog::level::info);
	}

	try {
		/*	*/
		fragcore::Library library("librenderdoc.so");

		/*	*/
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)library.getfunc("RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (&this->rdoc_api));

		assert(ret == 1);

	} catch (const std::exception &ex) {
		this->logger->warn(ex.what());
	}
}

void GLSampleWindow::captureScreenShot() {
	/*	*/
	const int screen_grab_width_size = this->width();
	const int screen_grab_height_size = this->height();

	if (glFramebufferSampleLocationsfvARB) {
	}

	/*	Make sure the frame is completed before extracing pixel data.	*/
	glFinish();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/*	Image required size.	*/
	const size_t alignSize = 4;
	const size_t pixelSizeInBytes = 3;
	const size_t imageSizeInBytes = static_cast<const size_t>(
		static_cast<size_t>(screen_grab_width_size * screen_grab_height_size) * pixelSizeInBytes);
	const size_t imageSizeInAlignBytes = Math::align<size_t>(imageSizeInBytes, alignSize);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);

	glBufferData(GL_PIXEL_PACK_BUFFER, imageSizeInAlignBytes, nullptr, GL_STREAM_READ);

	/*	Read framebuffer, and transfer the result to PBO, to allow DMA and less sync between frames.	*/

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_BGR, GL_UNSIGNED_BYTE, nullptr);

	void *pixelData = nullptr;

	/*	*/
	fragcore::Image image(screen_grab_width_size, screen_grab_height_size, fragcore::ImageFormat::RGB24);
	pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	image.setPixelData(pixelData, imageSizeInBytes);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	*/
	glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

	/*	offload the image process and start saving to filesystem.	*/
	if (image.getPixelData() && image.width() > 0 && image.height() > 0) {
		std::thread process_thread([&, image]() {
			/*	*/
			try {

				// Application and time
				time_t rawtime = 0;
				struct tm *timeinfo = nullptr;
				std::array<char, PATH_MAX> buffer{};
				/*	*/
				std::time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(buffer.data(), buffer.size(), "%d-%m-%Y %H:%M:%S", timeinfo);
				std::string str(buffer.data());

				fragcore::ImageLoader loader;
				const std::string filename = fragcore::SystemInfo::getApplicationName() + "-screenshot-" + str + ".jpg";
				FileSystem *filesystem = FileSystem::getFileSystem();
				loader.saveImage(filename, image, filesystem, fragcore::ImageLoader::FileFormat::Jpeg);

			} catch (const std::exception &ex) {
				this->getLogger().error("Failed to create ScreenShot {}", ex.what());
			}
		});

		/*	Allow to process indepdent from the main program.	*/
		process_thread.detach();
	}
}

void GLSampleWindow::setColorSpace(const glsample::ColorSpace srgb) {
	if (this->colorSpace != nullptr) {
		this->colorSpace->setColorSpace(srgb);
	}
}

glsample::ColorSpace GLSampleWindow::getColorSpace() const noexcept {
	if (this->colorSpace != nullptr) {
		return this->colorSpace->getColorSpace();
	}
	return ColorSpace::RawLinear;
}

void GLSampleWindow::vsync(const bool enable_vsync) { SDL_GL_SetSwapInterval(enable_vsync); }

bool GLSampleWindow::getVSync() const { return SDL_GL_GetSwapInterval(); }

void GLSampleWindow::enableRenderDoc(const bool status) {
	if (status) {
		RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;

		// rdoc_api_inter->LaunchReplayUI(1, "");
	}
}

void GLSampleWindow::launchRenderDoc() {

	RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;

	rdoc_api_inter->LaunchReplayUI(1, "");
}

bool GLSampleWindow::isRenderDocEnabled() noexcept {

	if (!this->rdoc_api) {
		return false;
	}

	/*	*/
	RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;

	if (!rdoc_api_inter->IsTargetControlConnected) {
		return false;
	}

	return rdoc_api_inter->IsTargetControlConnected();
}

void GLSampleWindow::captureDebugFrame() noexcept {
	if (this->isRenderDocEnabled()) {
		RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;
		//		rdoc_api_inter->SetCaptureTitle("");
		rdoc_api_inter->TriggerCapture();
	}
}

void GLSampleWindow::createDefaultFrameBuffer() {

	if (this->defaultFramebuffer == nullptr) {
		this->defaultFramebuffer = std::make_shared<glsample::FrameBuffer>();
		memset(defaultFramebuffer.get(), 0, sizeof(*this->defaultFramebuffer));
	}
}

void GLSampleWindow::updateDefaultFramebuffer() {

	/*	*/
	const unsigned int multi_sample_count = this->getResult()["multi-sample"].as<int>();
	/*	*/
	const std::string dynamicRange = this->getResult()["dynamic-range"].as<std::string>();

	GraphicFormat internal_color_format = GraphicFormat::R16G16B16A16_SFloat;
	GraphicFormat internal_depth_format = GraphicFormat::Depth_32Bit;

	/*	Override the default texture color format.	*/
	if (dynamicRange == "ldr") {
		internal_color_format = GraphicFormat::B8G8R8A8_UNorm;
	} else if (dynamicRange == "hdr" || dynamicRange == "hdr32") {
		internal_color_format = GraphicFormat::R32G32B32A32_SFloat;
	} else if (dynamicRange == "hdr16") {
		internal_color_format = GraphicFormat::R16G16B16A16_SFloat;
	}

	const int framebuffer_Width = this->width() * this->getSizeSSAFactor();
	const int framebuffer_Height = this->height() * this->getSizeSSAFactor();

	if (this->MMSAFrameBuffer) {
		const TextureDesc depthStencil = {
			.width = framebuffer_Width,
			.height = framebuffer_Height,
			.depth = 1,
			.graphicFormat = internal_depth_format,
			.nrSamples = multi_sample_count,
		};
		CommonUtil::updateFrameBuffer(this->MMSAFrameBuffer.get(),
									  {{
										  .width = framebuffer_Width,
										  .height = framebuffer_Height,
										  .depth = 1,
										  .graphicFormat = internal_color_format,
										  .nrSamples = multi_sample_count,

									  }},
									  &depthStencil);
	}

	if (this->defaultFramebuffer != nullptr) {
		const TextureDesc depthStencil = {
			.width = framebuffer_Width,
			.height = framebuffer_Height,
			.depth = 1,
			.graphicFormat = internal_depth_format,
			.nrSamples = 0,
		};
		CommonUtil::updateFrameBuffer(this->defaultFramebuffer.get(),
									  {{
										   .width = framebuffer_Width,
										   .height = framebuffer_Height,
										   .depth = 1,
										   .graphicFormat = internal_color_format,
										   .nrSamples = 0,
										   .numlevel = 4,
									   },
									   {
										   .width = framebuffer_Width,
										   .height = framebuffer_Height,
										   .depth = 1,
										   .graphicFormat = internal_color_format,
										   .nrSamples = 0,
										   .numlevel = 4,

									   },
									   {
										   .width = framebuffer_Width,
										   .height = framebuffer_Height,
										   .depth = 1,
										   .graphicFormat = internal_color_format,
										   .nrSamples = 0,
										   .numlevel = 4,
									   }},
									  &depthStencil);
	}
}

unsigned int GLSampleWindow::getDefaultFramebuffer() const noexcept {

	if (this->MMSAFrameBuffer) {
		GLboolean isEnabled = 0;
		glGetBooleanv(GL_MULTISAMPLE, &isEnabled);
		if (isEnabled) {
			return this->MMSAFrameBuffer->framebuffer;
		}
	}

	if (this->defaultFramebuffer) {
		return this->defaultFramebuffer->framebuffer;
	}
	return 0; /*	OpenGL Default FrameBuffer.	*/
}