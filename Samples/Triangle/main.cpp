#include "GLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <iostream>

class TriangleWindow : public GLWindow {
  public:
	TriangleWindow() : GLWindow(-1, -1, -1, -1) {}
	typedef struct _vertex_t {
		float pos[2];
		float color[3];
	} Vertex;

	const std::vector<Vertex> vertices = {
		{0.0f, -0.5f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f, 0.0f, 1.0f, 0.0f}, {-0.5f, 0.5f, 0.0f, 0.0f, 1.0f}};

	virtual void Release(void) override {}
	virtual void Initialize(void) override {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		/*	Load shader	*/

		/*	Create geomtry.	*/
	}

	virtual void draw(void) override {

		int w, h;
		getSize(&w, &h);

		glClear(GL_COLOR_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	}

	virtual void update(void) {}
};

int main(int argc, const char **argv) {
	try {

		//	OpenGLCore core(argc, argv);
		TriangleWindow w;

		w.run();
	} catch (std::exception &ex) {
		std::cerr << ex.what();
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
