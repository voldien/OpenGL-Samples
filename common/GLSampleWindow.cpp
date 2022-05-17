#include "GLSampleWindow.h"

// TODO add supprt for renderdoc

GLSampleWindow::GLSampleWindow() : nekomimi::MIMIWindow(nekomimi::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {
	this->enableDocking(false);

	this->getTimer().start();
}

void GLSampleWindow ::displayMenuBar() {}
void GLSampleWindow ::renderUI() {
	this->draw();

	this->frameCount++;
	this->getTimer().update();
	this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

	std::cout << "FPS " << getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed() << std::endl;
}
