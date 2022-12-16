#include "GLSampleWindow.h"
#include <ImageLoader.h>
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
}

void GLSampleWindow::displayMenuBar() {}
void GLSampleWindow::renderUI() {

	/*	*/
	glBeginQuery(GL_TIME_ELAPSED, this->queries[0]);
	glBeginQuery(GL_SAMPLES_PASSED, this->queries[1]);
	glBeginQuery(GL_PRIMITIVES_GENERATED, this->queries[2]);

	this->draw();

	/*	*/
	glEndQuery(GL_TIME_ELAPSED);
	glEndQuery(GL_SAMPLES_PASSED);
	glEndQuery(GL_PRIMITIVES_GENERATED);

	int nrPrimitives;
	int nrSamples;
	int time_elasped;
	glGetQueryObjectiv(this->queries[0], GL_QUERY_RESULT, &nrSamples);
	// glGetQueryObjectiv(this->queries[0], GL_QUERY_RESULT, &nrSamples);
	// glGetQueryObjectiv(this->queries[1], GL_QUERY_RESULT, &nrPrimitives);

	this->frameCount++;
	this->getTimer().update();
	this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

	std::cout << "FPS " << getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed() << std::endl;

	const Uint8 *state = SDL_GetKeyboardState(nullptr);
	if (state[SDL_SCANCODE_F12]) {
		captureScreenShot();
	}
}

void GLSampleWindow::setTitle(const std::string &title) {

	nekomimi::MIMIWindow::setTitle(title + " - OpenGL version " + getRenderInterface()->getAPIVersion());
}

void GLSampleWindow::captureScreenShot() {
	int screen_grab_width_size = this->width();
	int screen_grab_height_size = this->height();

	glFinish();

	// unsigned int pboBuffer;
	// glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr, GL_STREAM_READ);
	// glBufferData(GL_PIXEL_PACK_BUFFER, screen_grab_width_size * screen_grab_height_size * 4, nullptr,
	// GL_STREAM_READ);

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, screen_grab_width_size, screen_grab_height_size, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

	void *pixelData = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	// glDeleteBuffers(1, &pboBuffer);

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

	loader.saveImage(SystemInfo::getAppliationName() + "-screenshot-" + str + ".png", image);
}
