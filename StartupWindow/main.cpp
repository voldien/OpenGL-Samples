#include "GLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <iostream>

class StartUpWindow : public GLWindow {
  public:
	StartUpWindow() : GLWindow(-1, -1, -1, -1) {}

	virtual void Initialize(void) { glClearColor(0.2f, 0.2f, 0.2f, 1.0f); }

	virtual void draw(void) {

		int w, h;
		getSize(&w, &h);

		glClear(GL_COLOR_BUFFER_BIT);
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {
	try {

		//	OpenGLCore core(argc, argv);
		StartUpWindow w();

		w.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
