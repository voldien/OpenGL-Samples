#include "GLSampleWindow.h"

// TODO add supprt for renderdoc

GLSampleWindow::GLSampleWindow() : MIMIIMGUI::MIMIWindow(MIMIIMGUI::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {
	this->enableDocking(false);
}

void GLSampleWindow ::displayMenuBar() {}
void GLSampleWindow ::renderUI() {
	draw();

	this->frameCount++;
	this->getTimer().update();
	this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

	std::cout << "FPS " << getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed() << std::endl;
}
