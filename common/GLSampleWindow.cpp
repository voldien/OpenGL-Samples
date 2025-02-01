#include "GLSampleWindow.h"
#include "Common.h"
#include "Core/Library.h"
#include "Core/SystemInfo.h"
#include "FPSCounter.h"
#include "GLUIComponent.h"

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
		ImGui::Text("FPS %d", this->getRefSample().getFPSCounter().getFPS());
		ImGui::Text("FrameCount %zu", this->getRefSample().getFrameCount());
		ImGui::Text("Frame Index %zu", this->getRefSample().getFrameBufferIndex());
		ImGui::Text("Primitive %zu", this->getRefSample().prev_frame_primitive_count);
		ImGui::Text("Samples %zu", this->getRefSample().prev_frame_sample_count);

		bool renderDocEnable = this->getRefSample().isRenderDocEnabled();
		if (ImGui::Checkbox("RenderDoc", &renderDocEnable)) {
			this->getRefSample().enableRenderDoc(renderDocEnable);
		}
		/*	*/
		if (ImGui::Button("Capture Frame")) {
			this->getRefSample().captureDebugFrame();
		}
		ImGui::Text("WorkDirectory: %s", fragcore::SystemInfo::getCurrentDirectory().c_str());
		// ImGui::Checkbox("Logging: %s", fragcore::SystemInfo::getCurrentDirectory().c_str());

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
		}

		ImGui::SeparatorText("Color Space Settings");
		if (this->getRefSample().getColorSpaceConverter()) {
			const int item_selected_idx =
				(int)this->getRefSample().getColorSpace(); // Here we store our selection data as an index.

			std::string combo_preview_value = std::string(magic_enum::enum_name(this->getRefSample().getColorSpace()));
			ImGuiComboFlags flags = 0;
			if (ImGui::BeginCombo("ColorSpace", combo_preview_value.c_str(), flags)) {
				for (int n = 0; n < (int)ColorSpace::MaxColorSpaces; n++) {
					const bool is_selected = (item_selected_idx == n);

					if (ImGui::Selectable(magic_enum::enum_name((ColorSpace)n).data(), is_selected)) {
						this->getRefSample().setColorSpace((ColorSpace)n);
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
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

		/*	List all builtin post processing.	*/
		if (this->getRefSample().getPostProcessingManager() && ImGui::CollapsingHeader("Post Processing")) {

			ImGui::BeginGroup();
			PostProcessingManager *manager = this->getRefSample().getPostProcessingManager();
			/*	*/
			for (size_t post_index = 0; post_index < manager->getNrPostProcessing(); post_index++) {
				PostProcessing &postEffect = manager->getPostProcessing(post_index);

				ImGui::PushID(post_index);

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
			ImGui::Image(framebuffer->depthbuffer, ImVec2(256, 256), ImVec2(1, 1), ImVec2(0, 0));
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
	glGenQueries(sizeof(this->queries) / sizeof(this->queries[0]), &this->queries[0]);

	/*	Disable automatic framebuffer gamma correction, each application handle it manually.	*/
	glDisable(GL_FRAMEBUFFER_SRGB);

	/*	Disable multi sampling by default.	*/
	glDisable(GL_MULTISAMPLE);

	/*	*/
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	/*	*/
	if (glMaxShaderCompilerThreadsKHR) {
		glMaxShaderCompilerThreadsKHR(fragcore::SystemInfo::getCPUCoreCount() / 2);
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

	/*	*/
	bool usePostProcessing = true;
	if (this->colorSpace == nullptr) {

		// TODO: add try catch.
		this->colorSpace = new ColorSpaceConverter();
		this->colorSpace->initialize(getFileSystem());

		if (usePostProcessing) {
			this->postprocessingManager = new PostProcessingManager();

			SobelProcessing *sobelPostProcessing = new SobelProcessing();
			sobelPostProcessing->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*sobelPostProcessing);

			ColorGradePostProcessing *colorgrade = new ColorGradePostProcessing();
			colorgrade->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*colorgrade);

			BlurPostProcessing *blur = new BlurPostProcessing();
			blur->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*blur);

			PixelatePostProcessing *pixelate = new PixelatePostProcessing();
			pixelate->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*pixelate);

			GrainPostProcessing *grain = new GrainPostProcessing();
			grain->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*grain);

			SSAOPostProcessing *ssao = new SSAOPostProcessing();
			ssao->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*ssao);

			DepthOfFieldProcessing *depthOfField = new DepthOfFieldProcessing();
			depthOfField->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*depthOfField);

			MistPostProcessing *mistFog = new MistPostProcessing();
			mistFog->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*mistFog);

			SSSPostProcessing *sss = new SSSPostProcessing();
			sss->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*sss);

			VolumetricScatteringPostProcessing *volumetric = new VolumetricScatteringPostProcessing();
			volumetric->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*volumetric);

			BloomPostProcessing *bloom = new BloomPostProcessing();
			bloom->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*bloom);

			ChromaticAbberationPostProcessing *chromatic = new ChromaticAbberationPostProcessing();
			chromatic->initialize(getFileSystem());
			this->postprocessingManager->addPostProcessing(*chromatic);
		}
	}

	const size_t multi_sample_count = this->getResult()["multi-sample"].as<int>();
	const bool useFBO = true;
	if (this->MMSAFrameBuffer == nullptr && multi_sample_count > 0) {

		this->MMSAFrameBuffer = new glsample::FrameBuffer();
		memset(MMSAFrameBuffer, 0, sizeof(*this->MMSAFrameBuffer));
		Common::createFrameBuffer(MMSAFrameBuffer, 1);
	}
	if (this->defaultFramebuffer == nullptr && useFBO) {
		/*	*/
		this->defaultFramebuffer = new glsample::FrameBuffer();
		memset(defaultFramebuffer, 0, sizeof(*this->defaultFramebuffer));
		Common::createFrameBuffer(defaultFramebuffer, 3);
	}

	/*	Update if not internal default framebuffer	*/
	if (getDefaultFramebuffer() > 0) {
		/*	*/
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
	}

	{

		/*	*/
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());

		/*	Main Draw Callback.	*/
		this->draw();

		/*	Transfer Multisampled texture to FBO.	*/
		if (this->MMSAFrameBuffer && this->MMSAFrameBuffer->framebuffer == this->getDefaultFramebuffer()) {
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
		}

		/*	*/
		if (this->postprocessingManager) {
			/*	*/
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			this->postprocessingManager->render(
				this->defaultFramebuffer,
				{std::make_tuple(GBuffer::Albedo, this->defaultFramebuffer->attachments[0]),
				 std::make_tuple(GBuffer::Depth, this->defaultFramebuffer->depthbuffer),
				 std::make_tuple(GBuffer::IntermediateTarget, this->defaultFramebuffer->attachments[1]),
				 std::make_tuple(GBuffer::IntermediateTarget2, this->defaultFramebuffer->attachments[2])});
		}

		/*	Transfer last result to the default OpenGL Framebuffer.	*/
		if (this->defaultFramebuffer) {
			if (this->colorSpace) {
				this->colorSpace->convert(this->defaultFramebuffer->attachments[0]);
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, this->width(), this->height());
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		this->postDraw();
	}

	/*	Extract debugging information.	*/
	if (this->debugGL) {
		glEndQuery(GL_TIME_ELAPSED);
		glEndQuery(GL_SAMPLES_PASSED);
		glEndQuery(GL_PRIMITIVES_GENERATED);

		int nrPrimitives = 0, nrSamples = 0, time_elasped = 0;
		glGetQueryObjectiv(this->queries[0], GL_QUERY_RESULT, &time_elasped);
		glGetQueryObjectiv(this->queries[1], GL_QUERY_RESULT, &nrSamples);
		glGetQueryObjectiv(this->queries[2], GL_QUERY_RESULT, &nrPrimitives);

		this->prev_frame_sample_count = nrSamples;
		this->prev_frame_primitive_count = nrPrimitives;

		this->getLogger().debug("Samples: {} Primitives: {} Elapsed: {} ms", nrSamples, nrPrimitives,
								time_elasped / 100000.0f);
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
	if (this->debugGL) {
		this->getLogger().info("FPS: {} Elapsed Time: {}", this->getFPSCounter().getFPS(),
							   this->getTimer().getElapsed<float>());
	}
	this->getTimer().update();
}

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + this->getRenderInterface()->getAPIVersion());
}

void GLSampleWindow::debug(const bool enable) {
	fragcore::GLRendererInterface *interface =
		dynamic_cast<fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	interface->setDebug(enable);

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

	/*	*/
	const size_t imageSizeInBytes = static_cast<const size_t>(screen_grab_width_size * screen_grab_height_size * 3);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);

	if (glBufferStorage) {
		glBufferStorage(GL_PIXEL_PACK_BUFFER, imageSizeInBytes, nullptr,
						GL_DYNAMIC_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT);
	} else {
		glBufferData(GL_PIXEL_PACK_BUFFER, imageSizeInBytes, nullptr, GL_STREAM_READ);
	}

	// FIXME: make sure that the image format is used correctly.
	/*	Read framebuffer, and transfer the result to PBO, to allow DMA and less sync between frames.	*/
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_BGR, GL_UNSIGNED_BYTE, nullptr);

	static void *pixelData = nullptr;

	/*	*/
	if (glBufferStorage && pixelData == nullptr) {
		pixelData = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, imageSizeInBytes,
									 GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	}

	if (!glBufferStorage) {
		pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	*/
	glMemoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

	/*	offload the image process and saving to filesystem.	*/
	if (pixelData) {
		std::thread process_thread([&, imageSizeInBytes, screen_grab_width_size, screen_grab_height_size]() {
			/*	*/
			try {
				fragcore::Image image(screen_grab_width_size, screen_grab_height_size, fragcore::ImageFormat::RGB24);
				image.setPixelData(pixelData, imageSizeInBytes);

				// Application and time
				time_t rawtime = 0;
				struct tm *timeinfo = nullptr;
				char buffer[128];

				std::time(&rawtime);
				timeinfo = localtime(&rawtime);

				strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
				std::string str(buffer);

				fragcore::ImageLoader loader;
				loader.saveImage(fragcore::SystemInfo::getApplicationName() + "-screenshot-" + str + ".jpg", image,
								 fragcore::ImageLoader::FileFormat::Jpeg);

			} catch (const std::exception &ex) {
				this->getLogger().error("Failed to create ScreenShot {}", ex.what());
			}
		});

		/*	Allow to process indepdent from the main program.	*/
		process_thread.detach();
	}
}

void GLSampleWindow::setColorSpace(glsample::ColorSpace srgb) {
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

void GLSampleWindow::vsync(bool enable_vsync) { SDL_GL_SetSwapInterval(enable_vsync); }

void GLSampleWindow::enableRenderDoc(bool status) {
	if (status) {
	}
}

bool GLSampleWindow::isRenderDocEnabled() {
	if (!this->rdoc_api) {
		return false;
	}
	RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;

	rdoc_api_inter->IsTargetControlConnected;

	return true;
}

void GLSampleWindow::captureDebugFrame() noexcept {
	if (this->isRenderDocEnabled()) {
		RENDERDOC_API_1_1_2 *rdoc_api_inter = (RENDERDOC_API_1_1_2 *)this->rdoc_api;
		rdoc_api_inter->TriggerCapture();
	}
}

unsigned int GLSampleWindow::getShaderVersion() const {
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

	const unsigned int multi_sample_count = this->getResult()["multi-sample"].as<int>();
	if (this->MMSAFrameBuffer) {
		Common::updateFrameBuffer(this->MMSAFrameBuffer,
								  {{
									  .width = this->width(),
									  .height = this->height(),
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
									   .nrSamples = 0,

								   },
								   {
									   .width = this->width(),
									   .height = this->height(),
									   .depth = 1,
									   .nrSamples = 0,

								   },
								   {
									   .width = this->width(),
									   .height = this->height(),
									   .depth = 1,
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
	return 0;
}