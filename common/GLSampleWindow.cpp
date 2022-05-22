#include "GLSampleWindow.h"
#include <ImageLoader.h>
// TODO add supprt for renderdoc

GLSampleWindow::GLSampleWindow() : nekomimi::MIMIWindow(nekomimi::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {
	this->enableDocking(false);

	this->getTimer().start();
}

void GLSampleWindow::displayMenuBar() {}
void GLSampleWindow::renderUI() {
	this->draw();

	this->frameCount++;
	this->getTimer().update();
	this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

	std::cout << "FPS " << getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed() << std::endl;
}

void GLSampleWindow::captureScreenShot() {
	int screen_grab_width_size = this->width();
	int screen_grab_height_size = this->height();

	unsigned int pboBuffer;
	glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 3, nullptr, GL_STATIC_COPY);

	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	void *pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	Image image(screen_grab_width_size, screen_grab_height_size, TextureFormat::RGB24);
	image.setPixelData(pixelData, screen_grab_width_size * screen_grab_height_size * 3);
	ImageLoader loader;
	// loader.saveImage("screenshot.png", image);
}