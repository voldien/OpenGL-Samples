#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <iostream>

namespace glsample {

	/**
	 * @brief
	 */
	class StartUpWindow : public GLSampleWindow {
	  public:
		StartUpWindow() : GLSampleWindow() { this->setTitle("StartUp Window"); }
		void Release() override {}

		void Initialize() override {}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	Set clear color.	*/
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Setup viewport size of the default framebuffer.	*/
			glViewport(0, 0, width, height);

			/*	Clear the framebuffer color value.	*/
			glClear(GL_COLOR_BUFFER_BIT);
		}

		void update() override {}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::StartUpWindow> sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {
		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
