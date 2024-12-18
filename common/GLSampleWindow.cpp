#include "GLSampleWindow.h"
#include "Core/Library.h"
#include "Core/SystemInfo.h"
#include "FPSCounter.h"
#include "GLHelper.h"
#include "GLUIComponent.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "imgui.h"
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
	}

  private:
};

GLSampleWindow::GLSampleWindow()
	: nekomimi::MIMIWindow(nekomimi::MIMIWindow::GfxBackEnd::ImGUI_OpenGL), preWidth(this->width()),
	  preHeight(this->height()) {

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

	std::shared_ptr<SampleSettingComponent> settingComponent = std::make_shared<SampleSettingComponent>(*this);
	this->addUIComponent(settingComponent);
}

void GLSampleWindow::displayMenuBar() {}

void GLSampleWindow::renderUI() {

	const size_t multi_sample_count = this->getResult()["multi-sample"].as<int>();
	if (this->defaultFramebuffer == nullptr && multi_sample_count > 0) {
		/*	*/
		this->createDefaultFrameBuffer();
	}

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
		if (this->defaultFramebuffer) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);
		}

		/*	*/
		this->draw();

		if (this->defaultFramebuffer) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0, 0, this->width(), this->height());
			glBlitFramebuffer(0, 0, this->width(), this->height(), 0, 0, this->width(), this->height(),
							  GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
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
		std::thread process_thread([&, imageSizeInBytes, screen_grab_width_size, screen_grab_height_size, pixelData]() {
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

void GLSampleWindow::setColorSpace(bool srgb) {}
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
		this->defaultFramebuffer = new FrameBuffer();
		memset(defaultFramebuffer, 0, sizeof(*this->defaultFramebuffer));
	}

	glGenFramebuffers(1, &this->defaultFramebuffer->framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);

	glGenTextures(1, &this->defaultFramebuffer->attachement0);
	glGenTextures(1, &this->defaultFramebuffer->depthbuffer);

	this->updateDefaultFramebuffer();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLSampleWindow::updateDefaultFramebuffer() {
	if (this->defaultFramebuffer != nullptr) {

		const int multisamples = this->getResult()["multi-sample"].as<int>();

		const int width = this->width();
		const int height = this->height();

		GLenum texture_type = GL_TEXTURE_2D;
		if (multisamples > 0) {
			texture_type = GL_TEXTURE_2D_MULTISAMPLE;
			glEnable(GL_MULTISAMPLE);
		}

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebuffer->framebuffer);
		glBindTexture(texture_type, this->defaultFramebuffer->attachement0);
		if (multisamples > 0) {
			glTexImage2DMultisample(texture_type, multisamples, GL_RGBA, width, height, GL_TRUE);
		} else {
			glTexImage2D(texture_type, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		/*	Border clamped to max value, it makes the outside area.	*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0));

		FVALIDATE_GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f));

		FVALIDATE_GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));

		glBindTexture(texture_type, 0);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_type,
							   this->defaultFramebuffer->attachement0, 0);

		/*	*/
		glBindTexture(texture_type, this->defaultFramebuffer->depthbuffer);
		if (multisamples > 0) {
			glTexImage2DMultisample(texture_type, multisamples, GL_DEPTH_COMPONENT32, width, height, GL_TRUE);
		} else {
			glTexImage2D(texture_type, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
						 nullptr);
		}
		glBindTexture(texture_type, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_type, this->defaultFramebuffer->depthbuffer,
							   0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

int GLSampleWindow::getDefaultFramebuffer() const noexcept {
	if (defaultFramebuffer) {
		return this->defaultFramebuffer->framebuffer;
	}
	return 0;
}