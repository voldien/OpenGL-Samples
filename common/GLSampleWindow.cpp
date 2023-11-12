#include "GLSampleWindow.h"
#include <GLRendererInterface.h>
#include <ImageLoader.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <iostream>

// TODO add supprt for renderdoc
unsigned int pboBuffer;

GLSampleWindow::GLSampleWindow() : nekomimi::MIMIWindow(nekomimi::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {
	this->enableDocking(false);

	this->getTimer().start();

	this->getRenderInterface()->setDebug(true);
	// this->set

	int screen_grab_width_size = this->width();
	int screen_grab_height_size = this->height();

	glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	glGenQueries(sizeof(this->queries) / sizeof(this->queries[0]), &this->queries[0]);

	this->preWidth = this->width();
	this->preHeight = this->height();

	/*	*/
	// glEnable(GL_FRAMEBUFFER_SRGB);

	// glGetIntegerv( FRAMEBUFFER ,  &frameBufferCount);
}

void GLSampleWindow::displayMenuBar() {}

void GLSampleWindow::renderUI() {

	if (this->preWidth != this->width() || this->preHeight != this->height()) {
		glFinish();
		this->onResize(this->width(), this->height());
	}
	this->preWidth = this->width();
	this->preHeight = this->height();

	this->update();

	/*	*/
	if (this->debugGL) {
		glBeginQuery(GL_TIME_ELAPSED, this->queries[0]);
		glBeginQuery(GL_SAMPLES_PASSED, this->queries[1]);
		glBeginQuery(GL_PRIMITIVES_GENERATED, this->queries[2]);
	}

	/*	*/
	this->draw();

	/*	*/
	if (this->debugGL) {
		glEndQuery(GL_TIME_ELAPSED);
		glEndQuery(GL_SAMPLES_PASSED);
		glEndQuery(GL_PRIMITIVES_GENERATED);

		int nrPrimitives, nrSamples, time_elasped;
		glGetQueryObjectiv(this->queries[0], GL_QUERY_RESULT, &time_elasped);
		glGetQueryObjectiv(this->queries[1], GL_QUERY_RESULT, &nrSamples);
		glGetQueryObjectiv(this->queries[2], GL_QUERY_RESULT, &nrPrimitives);

		std::cout << "Samples: " << nrSamples << " Primitives: " << nrPrimitives << std::endl;
	}

	/*	*/
	this->frameCount++;
	this->frameBufferIndex = (this->frameBufferIndex + 1) % this->getFrameBufferCount();
	this->getTimer().update();
	this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

	/*	*/
	std::cout << "FPS " << this->getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed() << std::endl;

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

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + getRenderInterface()->getAPIVersion());
}

void GLSampleWindow::debug(bool enable) {
	fragcore::GLRendererInterface *interface =
		dynamic_cast<fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	interface->setDebug(enable);
}

void GLSampleWindow::captureScreenShot() {
	int screen_grab_width_size = this->width();
	int screen_grab_height_size = this->height();

	// Sync and fence.

	/*	Make sure the frame is completed before extracing pixel data.	*/
	glFinish();

	// unsigned int pboBuffer;
	// glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr, GL_STREAM_READ);
	// glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr,
	// GL_STREAM_READ);

	// FIXME: make sure that the image format is used correctly.
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

	void *pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	/*	offload the image process and saving to filesystem.	*/
	std::thread process_thread([screen_grab_width_size, screen_grab_height_size, pixelData]() {
		/*	*/
		Image image(screen_grab_width_size, screen_grab_height_size, TextureFormat::RGBA32);
		image.setPixelData(pixelData, screen_grab_width_size * screen_grab_height_size * 4);
		ImageLoader loader;

		// Application and time
		time_t rawtime;
		struct tm *timeinfo;
		char buffer[80];

		std::time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
		std::string str(buffer);

		loader.saveImage(SystemInfo::getApplicationName() + "-screenshot-" + str + ".png", image);
	});
	process_thread.detach();
}

unsigned int GLSampleWindow::getShaderVersion() const {
	const fragcore::GLRendererInterface *interface =
		dynamic_cast<const fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	const char *shaderVersion = interface->getShaderVersion(fragcore::ShaderLanguage::GLSL);
	unsigned int version = std::stoi(shaderVersion);
	return version;
}

bool GLSampleWindow::supportSPIRV() const {
	const fragcore::GLRendererInterface *interface =
		dynamic_cast<const fragcore::GLRendererInterface *>(this->getRenderInterface().get());
	return (interface->getShaderLanguage() & (fragcore::ShaderLanguage::SPIRV != 0));
}
