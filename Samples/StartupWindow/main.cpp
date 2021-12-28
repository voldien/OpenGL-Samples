#include "GLSampleWindow.h"
#include "GLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <iostream>

class StartUpWindow : public GLSampleWindow {
  public:
	StartUpWindow() : GLSampleWindow() {}
	virtual void Release() override {}

	virtual void Initialize() override {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		int w, h;
		getSize(&w, &h);
		// onResize(w, h);
	}

	virtual void draw() override {

		int w, h;
		getSize(&w, &h);

		glClear(GL_COLOR_BUFFER_BIT);
	}

	// virtual void onResize(int width, int height) override { glViewport(0, 0, width, height); }

  private:
};

int main(int argc, const char **argv) {
	try {

		StartUpWindow startWindow;
		startWindow.run();

	} catch (const std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
