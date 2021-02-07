#include"SampleCommon.h"
#include<GL/glew.h>
#include<SDL2/SDL.h>
#include<SDL2/SDL_video.h>

static const char *minRequiredExtensions[] = {
		/*  Shaders.    */
		"GL_ARB_fragment_shader",
		"GL_ARB_vertex_shader",
		"GL_ARB_shader_objects",
		/*  Shader features.    */
		"GL_ARB_explicit_attrib_location",

		/*  Buffer objects. */
		"GL_ARB_vertex_buffer_object",
		"GL_ARB_pixel_buffer_object",

		"GL_ARB_multitexture",
		/*  Textures.   */

		/*  */
		"GL_ARB_vertex_array_object",
};

const unsigned int numMinReqExtensions = sizeof(minRequiredExtensions) / sizeof(minRequiredExtensions[0]);

void initWindowManager(){

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0){
//		throw (fvformatf("SDL_InitSubSystem failed, %s.\n", SDL_GetError()));
	}
}




SDL_Window *createWindow(const char *ctitle) {

	SDL_Window* window = NULL;      /*	*/
	SDL_DisplayMode mode;
	int winres[2];
	int winpos[2];

	// /*	Set window resolution.	*/
	// if (g_winres[0] == -1 && g_winres[1] == -1) {

	// 	SDL_DisplayMode mode;
	// 	SDL_GetCurrentDisplayMode(0, &mode);
	// 	g_winres[0] = mode.w / 2;
	// 	g_winres[1] = mode.h / 2;
	// }

	// /*	Set window position.	*/
	// if (g_winpos[0] == -1 && g_winpos[1] == -1) {
	// 	SDL_DisplayMode mode;
	// 	SDL_GetCurrentDisplayMode(0, &mode);
	// 	g_winpos[0] = mode.w / 4;
	// 	g_winpos[1] = mode.h / 4;
	// }

	SDL_GetCurrentDisplayMode(0, &mode);
	winres[0] = mode.w / 2;
	winres[1] = mode.h / 2;
	winpos[0] = mode.w / 4;
	winpos[1] = mode.h / 4;

	/*	Create window.	*/
	window = SDL_CreateWindow(ctitle,
			winpos[0], winpos[1],
			winres[0], winres[1],
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE );

	if (window == NULL) {
		fprintf(stderr, "Failed to create window, %s.\n", SDL_GetError());
		//goto error;
	}

	SDL_GLContext* context = SDL_GL_CreateContext(window);
	if (context == NULL) {
		fprintf(stderr, "Failed create OpenGL core context, %s.\n", SDL_GetError());

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		context = SDL_GL_CreateContext(window);
		if (context == NULL) {
			fprintf(stderr, "Failed to create OpenGL context current, %s.\n", SDL_GetError());
			//goto error;
		}
	}
	if (SDL_GL_MakeCurrent(window, context) < 0) {
		fprintf(stderr, "Failed to set OpenGL context current, %s.\n", SDL_GetError());
		//goto error;
	}

	for (int i = 0; i < numMinReqExtensions; i++) {
		const char *extension = minRequiredExtensions[i];
		if (!glewIsExtensionSupported(extension)) {
			// throw RuntimeException(
			// 		fvformatf("Non supported GPU - %s using OpenGL version: %s\nGLSL: %s", extension, getVersion(),
			// 		          glGetString(GL_SHADING_LANGUAGE_VERSION)));
		}
	}

	/*	Init GLEW library.	*/
	GLenum status = glewInit();
	if (status != GLEW_OK) {
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		//throw RuntimeException(fvformatf("Could not Initialize GLEW - %s.", glewGetErrorString(status)));
	}


	/*	Set OpenGL state.	*/
	SDL_GL_SetSwapInterval(SDL_TRUE);   /*	Enable vsync.	*/
	glDepthMask(GL_FALSE);              /*	Depth mask isn't needed.	*/
	glDepthFunc(GL_LESS);               /**/
	glEnable(GL_DITHER);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);           /*	Depth isn't needed.	*/
	glDisable(GL_STENCIL_TEST);         /*	Stencil isn't needed.	*/
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glCullFace(GL_FRONT_AND_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

	return window;
}

void deleteWindow(SDL_Window* window){
	SDL_GLContext* context = SDL_GL_GetCurrentContext();
	SDL_GL_MakeCurrent(window, NULL);
	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
}