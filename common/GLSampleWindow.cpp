#include "GLSampleWindow.h"
#include "FPSCounter.h"
#include "spdlog/common.h"
#include <GLRendererInterface.h>
#include <ImageLoader.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>

using namespace glsample;

// TODO add support for renderdoc
unsigned int pboBuffer;

GLSampleWindow::GLSampleWindow() : nekomimi::MIMIWindow(nekomimi::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {

	/* Create logger	*/
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	stdout_sink->set_level(spdlog::level::trace);
	stdout_sink->set_color_mode(spdlog::color_mode::always);
	stdout_sink->set_pattern("[%Y-%m-%d %T.%e] [%^%l%$] %v");
	stdout_sink->set_pattern("%g:%# [%^%l%$] %v");

	this->logger = new spdlog::logger("glsample", {stdout_sink});
	this->logger->set_level(spdlog::level::trace);

	/*	*/
	this->enableDocking(false);

	/*	*/
	this->fpsCounter = FPSCounter<float>(50, this->getTimer().getTimeResolution());
	this->getTimer().start();

	this->getRenderInterface()->setDebug(true);
	// this->set

	const int screen_grab_width_size = this->width();
	const int screen_grab_height_size = this->height();

	/*	*/
	glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	*/
	glGenQueries(sizeof(this->queries) / sizeof(this->queries[0]), &this->queries[0]);

	/*	*/
	this->preWidth = this->width();
	this->preHeight = this->height();

	/*	*/
	glDisable(GL_FRAMEBUFFER_SRGB);
}

void GLSampleWindow::displayMenuBar() {}

void GLSampleWindow::renderUI() {

	/*	Make sure all commands are flush before resizing.	*/
	if (this->preWidth != this->width() || this->preHeight != this->height()) {
		/*	Finish all commands before starting resizing buffers and etc.	*/
		glFinish();

		this->onResize(this->width(), this->height());
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

	/*	*/
	this->draw();

	/*	Extract debugging information.	*/
	if (this->debugGL) {
		glEndQuery(GL_TIME_ELAPSED);
		glEndQuery(GL_SAMPLES_PASSED);
		glEndQuery(GL_PRIMITIVES_GENERATED);

		int nrPrimitives, nrSamples, time_elasped;
		glGetQueryObjectiv(this->queries[0], GL_QUERY_RESULT, &time_elasped);
		glGetQueryObjectiv(this->queries[1], GL_QUERY_RESULT, &nrSamples);
		glGetQueryObjectiv(this->queries[2], GL_QUERY_RESULT, &nrPrimitives);

		this->getLogger().debug("Samples: {} Primitives: {} Elapsed: {}", nrSamples, nrPrimitives,
								time_elasped / 100000.0f);
	}

	/*	*/
	this->frameCount++;
	this->frameBufferIndex = (this->frameBufferIndex + 1) % this->getFrameBufferCount();
	this->getTimer().update();
	this->getFPSCounter().update(this->getTimer().getElapsed<float>());

	/*	*/
	if (this->debugGL) {
		this->getLogger().info("FPS: {} Elapsed Time: {}", this->getFPSCounter().getFPS(),
							   this->getTimer().getElapsed<float>());
	}

	{
		/*	*/
		const Uint8 *state = SDL_GetKeyboardState(nullptr);

		/*	Check if screenshot button pressed.	*/
		if (state[SDL_SCANCODE_F12]) {
			this->captureScreenShot();
		}

		/*	Enter fullscreen via short command.	*/
		if (state[SDL_SCANCODE_RETURN] && (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])) {
			this->setFullScreen(!this->isFullScreen());
		}
	}
}

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + this->getRenderInterface()->getAPIVersion());
}

void GLSampleWindow::debug(bool enable) {
	fragcore::GLRendererInterface *interface =
		dynamic_cast<fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	interface->setDebug(enable);

	if (enable) {
		this->logger->set_level(spdlog::level::trace);
	} else {
		this->logger->set_level(spdlog::level::info);
	}
}

void GLSampleWindow::captureScreenShot() {
	/*	*/
	const int screen_grab_width_size = this->width();
	const int screen_grab_height_size = this->height();

	/*	Make sure the frame is completed before extracing pixel data.	*/
	glFinish();

	/*	*/
	const size_t imageSizeInBytes = screen_grab_width_size * screen_grab_height_size * 3;
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, imageSizeInBytes, nullptr, GL_STREAM_READ);

	// FIXME: make sure that the image format is used correctly.
	/*	Read framebuffer, and transfer the result to PBO, to allow DMA and less sync between frames.	*/
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_BGR, GL_UNSIGNED_BYTE, nullptr);

	void *pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	offload the image process and saving to filesystem.	*/
	std::thread process_thread([imageSizeInBytes, screen_grab_width_size, screen_grab_height_size, pixelData]() {
		/*	*/
		fragcore::Image image(screen_grab_width_size, screen_grab_height_size, fragcore::TextureFormat::RGB24);
		image.setPixelData(pixelData, imageSizeInBytes);

		// Application and time
		time_t rawtime;
		struct tm *timeinfo;
		char buffer[128];

		std::time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
		std::string str(buffer);

		fragcore::ImageLoader loader;
		loader.saveImage(fragcore::SystemInfo::getApplicationName() + "-screenshot-" + str + ".jpg", image,
						 fragcore::ImageLoader::FileFormat::Jpeg);
	});

	/*	Allow to process indepdent from the main program.	*/
	process_thread.detach();
}

void GLSampleWindow::setColorSpace(bool srgb) {}

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
