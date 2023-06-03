#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <iostream>

class StartUpWindow : public GLSampleWindow {
  public:
	StartUpWindow() : GLSampleWindow() { this->setTitle("StartUp Window"); }
	virtual void Release() override {}

	virtual void Initialize() override {}

	virtual void draw() override {

		int width, height;
		this->getSize(&width, &height);

		/*	Set clear color.	*/
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

		/*	Setup viewport size of the default framebuffer.	*/
		glViewport(0, 0, width, height);

		/*	Clear the framebuffer color value.	*/
		glClear(GL_COLOR_BUFFER_BIT);
	}

	virtual void update() override {}
};

int main(int argc, const char **argv) {
	try {
		GLSample<StartUpWindow> sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
