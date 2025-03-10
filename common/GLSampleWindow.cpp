#include "GLSampleWindow.h"
#include "Common.h"
#include "Core/Library.h"
#include "Core/SystemInfo.h"
#include "FPSCounter.h"
#include "GLUIComponent.h"

#include "GraphicFormat.h"
#include "PostProcessing/BloomPostProcessing.h"
#include "PostProcessing/BlurPostProcessing.h"
#include "PostProcessing/ChromaticAbberationPostProcessing.h"
#include "PostProcessing/ColorGradePostProcessing.h"
#include "PostProcessing/ColorSpaceConverter.h"
#include "PostProcessing/DepthOfFieldPostProcessing.h"
#include "PostProcessing/GrainPostProcessing.h"
#include "PostProcessing/MistPostProcessing.h"
#include "PostProcessing/PixelatePostProcessing.h"
#include "PostProcessing/PostProcessing.h"
#include "PostProcessing/PostProcessingManager.h"
#include "PostProcessing/SSAOPostProcessing.h"
#include "PostProcessing/SSSPostProcessing.h"
#include "PostProcessing/SobelPostProcessing.h"

#include "PostProcessing/VolumetricScattering.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "SampleHelper.h"
#include "imgui.h"
#include "magic_enum.hpp"
#include "spdlog/common.h"
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

using namespace glsample;

unsigned int pboBuffer;

class SampleSettingComponent : public GLUIComponent<GLSampleWindow> {
  public:
	SampleSettingComponent(GLSampleWindow &base) : GLUIComponent(base) {}

	void draw() override {

		ImGui::SeparatorText("Debug");
		bool isDebug = this->getRefSample().isDebug();
		if (ImGui::Checkbox("Debug", &isDebug)) {
			this->getRefSample().debug(isDebug);
		}

		ImGui::BeginGroup();
		ImGui::Text("FPS %d", this->getRefSample().getFPSCounter().getFPS());
		ImGui::Text("FrameCount %zu", this->getRefSample().getFrameCount());
		ImGui::Text("Frame Index %zu", this->getRefSample().getFrameBufferIndex());

		ImGui::Text("Elapsed Time %.6f ms",
					(float)this->getRefSample().time_elapsed / (float)this->getRefSample().time_resolution);
		ImGui::Text("Primitive %zu", this->getRefSample().debug_prev_frame_primitive_count);
		ImGui::Text("Samples %zu", this->getRefSample().debug_prev_frame_sample_count);
		ImGui::Text("CS invocation %zu", this->getRefSample().debug_prev_frame_cs_invocation_count);
		ImGui::Text("Frag invocation %zu", this->getRefSample().debug_prev_frame_frag_invocation_count);
		ImGui::Text("Vertex invocation %zu", this->getRefSample().debug_prev_frame_vertex_invocation_count);
		ImGui::Text("Geometry invocation %zu", this->getRefSample().debug_prev_frame_geometry_invocation_count);

		ImGui::EndGroup();

		bool renderDocEnable = this->getRefSample().isRenderDocEnabled();
		if (ImGui::Checkbox("RenderDoc", &renderDocEnable)) {
			this->getRefSample().enableRenderDoc(renderDocEnable);
		}
		/*	*/
		if (ImGui::Button("Capture Frame")) {
			this->getRefSample().captureDebugFrame();
		}
		ImGui::Text("WorkDirectory: %s", fragcore::SystemInfo::getCurrentDirectory().c_str());
		bool isVsync = false;
		if (ImGui::Checkbox("VSync", &isVsync)) {
			this->getRefSample().vsync(isVsync);
		}

		ImGui::BeginDisabled(this->getRefSample().getDefaultFramebuffer() == 0);
		if (this->getRefSample().getDefaultFramebuffer() > 0) {
			GLboolean isEnabled = 0;
			glGetBooleanv(GL_MULTISAMPLE, &isEnabled);
			if (ImGui::Checkbox("MultiSampling (MSAA)", (bool *)&isEnabled)) {

				if (isEnabled) {
					glEnable(GL_MULTISAMPLE);
				} else {
					glDisable(GL_MULTISAMPLE);
				}
			}

			float min_sample = 0;
			glGetFloatv(GL_MIN_SAMPLE_SHADING_VALUE, &min_sample);
			if (ImGui::DragFloat("Min Sample", &min_sample, 1, 0, 1)) {
				glEnable(GL_SAMPLE_SHADING);
				glMinSampleShading(min_sample);
			}
		}
		ImGui::EndDisabled();

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
		ImGui::BeginDisabled(this->getRefSample().getDefaultFramebuffer() == 0);
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

				bool isEnabled = manager->isEnabled(post_index);
				if (ImGui::Checkbox("Enabled", &isEnabled)) {
					manager->enablePostProcessing(post_index, isEnabled);
				}
				float enabled_intensity = postEffect.getIntensity();
				if (ImGui::SliderFloat("Intensity", &enabled_intensity, 0.0, 1)) {
					postEffect.setItensity(enabled_intensity);
				}

				postEffect.renderUI();

				ImGui::EndDisabled();
				ImGui::PopID();
			}

			ImGui::EndGroup();
		}

		/*	Display All Framebuffer textures.	*/
		const glsample::FrameBuffer *framebuffer = this->getRefSample().getFrameBuffer();
		if (ImGui::CollapsingHeader("FrameBuffer Texture Targets") && framebuffer) {
			for (size_t attach_index = 0; attach_index < framebuffer->nrAttachments; attach_index++) {
				ImGui::Image(static_cast<ImTextureID>(framebuffer->attachments[attach_index]), ImVec2(256, 256),
							 ImVec2(1, 1), ImVec2(0, 0));
			}
			ImGui::Image(static_cast<ImTextureID>(framebuffer->depthbuffer), ImVec2(256, 256), ImVec2(1, 1),
						 ImVec2(0, 0));
		}
	}

  private:
};

GLSampleWindow::GLSampleWindow()
	: nekomimi::MIMIWindow(nekomimi::GfxBackEnd::ImGUI_OpenGL), preWidth(this->width()), preHeight(this->height()) {

	/* Create logger	*/
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_level(spdlog::level::trace);
	stdout_sink->set_color_mode(spdlog::color_mode::always);
	stdout_sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
	stdout_sink->set_pattern("%g:%# [%^%l%$] %v");

	/*	*/
	this->logger = new spdlog::logger("glsample", {stdout_sink});
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

GLSampleWindow::~GLSampleWindow() {
	delete this->colorSpace;
	delete this->postprocessingManager;
	/*	*/
}

void GLSampleWindow::internalInit() {

	const size_t use_post_process = this->getResult()["use-postprocessing"].as<bool>();
	const size_t multi_sample_count = this->getResult()["multi-sample"].as<int>();
	const bool useFBO = multi_sample_count > 0 || use_post_process || true; // TODO: fix conditions.

	if (this->colorSpace == nullptr) {

		// TODO: add try catch.
		this->colorSpace = new ColorSpaceConverter();
		this->colorSpace->initialize(getFileSystem());

		if (use_post_process) {
			this->postprocessingManager = new PostProcessingManager();

			SSAOPostProcessing *ssao = new SSAOPostProcessing();
			ssao->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*ssao);

			SSSPostProcessing *sss = new SSSPostProcessing();
			sss->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*sss);

			SobelProcessing *sobelPostProcessing = new SobelProcessing();
			sobelPostProcessing->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*sobelPostProcessing);

			ColorGradePostProcessing *colorgrade = new ColorGradePostProcessing();
			colorgrade->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*colorgrade);

			PixelatePostProcessing *pixelate = new PixelatePostProcessing();
			pixelate->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*pixelate);

			GrainPostProcessing *grain = new GrainPostProcessing();
			grain->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*grain);

			DepthOfFieldProcessing *depthOfField = new DepthOfFieldProcessing();
			depthOfField->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*depthOfField);

			MistPostProcessing *mistFog = new MistPostProcessing();
			mistFog->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*mistFog);

			VolumetricScatteringPostProcessing *volumetric = new VolumetricScatteringPostProcessing();
			volumetric->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*volumetric);

			BlurPostProcessing *blur = new BlurPostProcessing();
			blur->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*blur);

			BloomPostProcessing *bloom = new BloomPostProcessing();
			bloom->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*bloom);

			ChromaticAbberationPostProcessing *chromatic = new ChromaticAbberationPostProcessing();
			chromatic->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*chromatic);
		}
	}

	/*	Multi sampling.	*/
	if (this->MMSAFrameBuffer == nullptr && multi_sample_count > 0) {

		this->MMSAFrameBuffer = new glsample::FrameBuffer();
		memset(MMSAFrameBuffer, 0, sizeof(*this->MMSAFrameBuffer));
		Common::createFrameBuffer(MMSAFrameBuffer, 1);
	}

	/*	Framebuffer.	*/
	if (this->defaultFramebuffer == nullptr && useFBO) {
		this->defaultFramebuffer = new glsample::FrameBuffer();
		memset(defaultFramebuffer, 0, sizeof(*this->defaultFramebuffer));
		Common::createFrameBuffer(defaultFramebuffer, 3);
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
			glViewport(0, 0, this->width(), this->height());

			/*	*/
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
			/*	*/
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glPopDebugGroup();
		}

		/*	*/
		if (this->postprocessingManager) {
			const std::string postStage = "post processing";
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, postStage.size(), postStage.data());
			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			this->postprocessingManager->render(
				this->defaultFramebuffer,
				/*	Setup References.	*/
				{std::make_tuple<const GBuffer, const unsigned int &>(GBuffer::Albedo,
																	  this->defaultFramebuffer->attachments[0]),
				 std::make_tuple<const GBuffer, const unsigned int &>(GBuffer::Depth,
																	  this->defaultFramebuffer->depthbuffer),
				 std::make_tuple<const GBuffer, const unsigned int &>(GBuffer::IntermediateTarget,
																	  this->defaultFramebuffer->attachments[1]),
				 std::make_tuple<const GBuffer, const unsigned int &>(GBuffer::IntermediateTarget2,
																	  this->defaultFramebuffer->attachments[2])});
			glPopDebugGroup();
		}

		/*	Transfer last result to the default OpenGL Framebuffer.	*/
		if (this->defaultFramebuffer) {
			if (this->colorSpace) {
				const std::string ColorSpaceConverterStage = "Color Space Conversion";
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, ColorSpaceConverterStage.length(),
								 ColorSpaceConverterStage.c_str());
				this->colorSpace->render(this->defaultFramebuffer->attachments[0]);
				glPopDebugGroup();
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, this->width(), this->height());
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, sizeof("Post Draw"), "Post Draw");
		this->postDraw();
		glPopDebugGroup();
	}

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
		glGetQueryObjectui64v(this->queries[0], GL_QUERY_RESULT, &time_elapsed);
		glGetQueryObjectui64v(this->queries[1], GL_QUERY_RESULT, &nrSamples);
		glGetQueryObjectui64v(this->queries[2], GL_QUERY_RESULT, &nrPrimitives);
		glGetQueryObjectui64v(this->queries[3], GL_QUERY_RESULT, &this->debug_prev_frame_cs_invocation_count);
		glGetQueryObjectui64v(this->queries[4], GL_QUERY_RESULT, &this->debug_prev_frame_frag_invocation_count);
		glGetQueryObjectui64v(this->queries[5], GL_QUERY_RESULT, &this->debug_prev_frame_vertex_invocation_count);
		glGetQueryObjectui64v(this->queries[6], GL_QUERY_RESULT, &this->debug_prev_frame_geometry_invocation_count);

		this->debug_prev_frame_sample_count = nrSamples;
		this->debug_prev_frame_primitive_count = nrPrimitives;

		this->getLogger().debug("Samples: {} Primitives: {} Elapsed: {} ms", nrSamples, nrPrimitives,
								(float)time_elapsed / (float)this->time_resolution);
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
		if (this->getInput().getKeyPressed(SDL_SCANCODE_RETURN) &&
			(this->getInput().getKeyPressed(SDL_SCANCODE_LCTRL) ||
			 this->getInput().getKeyPressed(SDL_SCANCODE_RCTRL))) {
			this->setFullScreen(!this->isFullScreen());
		}

		/*	*/
		if (this->getInput().getKeyReleased(SDL_SCANCODE_F1)) {
			this->enableImGUI(!this->isEnabled());
		}

		if (this->getInput().getKeyPressed(SDL_SCANCODE_F9)) {
			this->captureDebugFrame();
		}
		if (this->getInput().getKeyPressed(SDL_SCANCODE_F1)) {
			// this->setStatusBar(true);
		}
	}

	this->getFPSCounter().update(this->getTimer().getElapsed<float>());

	/*	*/
	this->getLogger().info("FPS: {} Elapsed Time: {} ({} ms)", this->getFPSCounter().getFPS(),
						   this->getTimer().getElapsed<float>(), this->getTimer().deltaTime<float>() * 1000);
	this->getTimer().update();
}

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + this->getRenderInterface()->getAPIVersion());
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
		fragcore::Library library("librenderdoc.so");
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)library.getfunc("RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&this->rdoc_api);
		assert(ret == 1);

	} catch (const std::exception &ex) {
		this->logger->warn(ex.what());
	}
}

void GLSampleWindow::captureScreenShot() {
	/*	*/
	const int screen_grab_width_size = this->width();
	const int screen_grab_height_size = this->height();

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

	// FIXME: make sure that the image format is used correctly.
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
				char buffer[256];
				/*	*/
				std::time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
				std::string str(buffer);

				fragcore::ImageLoader loader;
				const std::string filename = fragcore::SystemInfo::getApplicationName() + "-screenshot-" + str + ".jpg";
				loader.saveImage(filename, image, fragcore::ImageLoader::FileFormat::Jpeg);

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

void GLSampleWindow::enableRenderDoc(const bool status) {
	if (status) {
	}
}

bool GLSampleWindow::isRenderDocEnabled() {
	if (!this->rdoc_api) {
		return false;
	}
	RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;

	return rdoc_api_inter->IsTargetControlConnected();
}

void GLSampleWindow::captureDebugFrame() noexcept {
	if (this->isRenderDocEnabled()) {
		RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;
		rdoc_api_inter->TriggerCapture();
	}
}

unsigned int GLSampleWindow::getShaderVersion() const {
	const int glsl_version = this->getResult()["glsl-version"].as<int>();

	/*	Override glsl version.	*/
	if (glsl_version >= 0) {
		return glsl_version;
	}

	const fragcore::GLRendererInterface *interface =
		dynamic_cast<const fragcore::GLRendererInterface *>(this->getRenderInterface().get());

	const char *shaderVersion = interface->getShaderVersion(fragcore::ShaderLanguage::GLSL);

	const unsigned int version = std::stoi(shaderVersion);
	return version;
}

bool GLSampleWindow::supportSPIRV() const {
	const fragcore::GLRendererInterface *interface =
		dynamic_cast<const fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	return (interface->getShaderLanguage() & (fragcore::ShaderLanguage::SPIRV != 0));
}

void GLSampleWindow::createDefaultFrameBuffer() {

	if (this->defaultFramebuffer == nullptr) {
		this->defaultFramebuffer = new glsample::FrameBuffer();
		memset(defaultFramebuffer, 0, sizeof(*this->defaultFramebuffer));
	}
}

void GLSampleWindow::updateDefaultFramebuffer() {

	/*	*/
	const unsigned int multi_sample_count = this->getResult()["multi-sample"].as<int>();
	/*	*/
	const std::string dynamicRange = this->getResult()["dynamic-range"].as<std::string>();

	GraphicFormat internal_format = GraphicFormat::R16G16B16A16_SFloat;
	if (dynamicRange == "ldr") {
		internal_format = GraphicFormat::B8G8R8A8_UNorm;
	} else if (dynamicRange == "hdr" || dynamicRange == "hdr32") {
		internal_format = GraphicFormat::R32G32B32A32_SFloat;
	} else if (dynamicRange == "hdr16") {
		internal_format = GraphicFormat::R16G16B16A16_SFloat;
	}

	if (this->MMSAFrameBuffer) {
		Common::updateFrameBuffer(this->MMSAFrameBuffer,
								  {{
									  .width = this->width(),
									  .height = this->height(),
									  .graphicFormat = internal_format,
									  .nrSamples = multi_sample_count,

								  }},
								  {
									  .width = this->width(),
									  .height = this->height(),
									  .nrSamples = multi_sample_count,
								  });
	}

	if (this->defaultFramebuffer != nullptr) {
		Common::updateFrameBuffer(this->defaultFramebuffer,
								  {{
									   .width = this->width(),
									   .height = this->height(),
									   .depth = 1,
									   .graphicFormat = internal_format,
									   .nrSamples = 0,

								   },
								   {
									   .width = this->width(),
									   .height = this->height(),
									   .depth = 1,
									   .graphicFormat = internal_format,
									   .nrSamples = 0,

								   },
								   {
									   .width = this->width(),
									   .height = this->height(),
									   .depth = 1,
									   .graphicFormat = internal_format,
									   .nrSamples = 0,
								   }},
								  {
									  .width = this->width(),
									  .height = this->height(),
									  .nrSamples = 0,
								  });
	}
}

int GLSampleWindow::getDefaultFramebuffer() const noexcept {

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