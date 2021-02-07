#include<SDL2/SDL.h>
#include<SDL2/SDL_video.h>
#include<GL/glew.h>
#include"SampleCommon.h"

int main(int argc, const char** argv) {

	int isAlive = 1;
	int visible = 0;
	SDL_Event event = {};


	initWindowManager();

	/*	*/
	SDL_Window* window = createWindow("OpenGL Window");
	SDL_ShowWindow(window);

	glClearColor(0.5f,0.5f,0.5f,1.0f);


	while (isAlive) {
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_APP_TERMINATING:
			case SDL_QUIT:
				isAlive = 0;
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.type) {
				case SDL_WINDOWEVENT_CLOSE:
					isAlive = SDL_FALSE;
					goto finish;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					glViewport(0, 0, event.window.data1, event.window.data2);
					printf("viewport: %dx%d\n", event.window.data1, event.window.data2);
					break;
				case SDL_WINDOWEVENT_HIDDEN:
					visible = 0;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_SHOWN:
					visible = 1;
					break;
				}
				/* code */
				break;
			default:
				break;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(window);
	}

finish:
	deleteWindow(window);
	SDL_Quit();
}
