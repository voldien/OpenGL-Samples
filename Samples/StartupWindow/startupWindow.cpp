#include "GLSampleWindow.h"
#include <GL/glew.h>
#include <iostream>

class StartUpWindow : public GLSampleWindow {
  public:
	StartUpWindow() : GLSampleWindow() { this->setTitle("StartUp Window"); }
	virtual void Release() override {}

	virtual void Initialize() override {}

	virtual void draw() override {

		int w, h;
		getSize(&w, &h);

		/*	Set clear color.	*/
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

		/*	Setup viewport size of the default framebuffer.	*/
		glViewport(0, 0, w, h);

		/*	Clear the framebuffer color value.	*/
		glClear(GL_COLOR_BUFFER_BIT);
	}

	virtual void update() override {}
};

int main(int argc, const char **argv) {
	try {
		GLSample<StartUpWindow> sample(argc, argv);
		sample.run();
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
