#include "GLWindow.h"
#include <GL/glew.h>
#include <cassert>
#include <fmt/format.h>
#include <iostream>
#include <stdexcept>

GLWindow::~GLWindow() { this->close(); }

GLWindow::GLWindow(int x, int y, int width, int height) {

	/**/
	if (SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error(fmt::format("Failed to init subsystem {}", SDL_GetError()));
	}

	SDL_DisplayMode displaymode;
	SDL_GetCurrentDisplayMode(0, &displaymode);
	if (x == -1 && y == -1) {
		x = displaymode.w / 4;
		y = displaymode.h / 4;
	}

	if (width == -1 && height == -1) {
		width = displaymode.w / 2;
		height = displaymode.h / 2;
	}

	/*  Create Vulkan window.   */
	this->window = SDL_CreateWindow("OpenGL Sample", x, y, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		throw std::runtime_error(fmt::format("failed create window - {}", SDL_GetError()));
	}

	createSwapChain();

	/*	Init GLEW library.	*/
	int status = glewInit();
	if (status != GLEW_OK) {
		SDL_GL_DeleteContext(this->glcontext);
		SDL_DestroyWindow(window);
		// throw cxxexcept::RuntimeException("Could not Initialize GLEW - {}.", glewGetErrorString(status));
	}

	/*	*/
	this->Initialize();
}

void GLWindow::swapBuffer() { SDL_GL_SwapWindow(this->window); }

void GLWindow::createSwapChain() { glcontext = SDL_GL_CreateContext(this->window); }

void GLWindow::recreateSwapChain() {

	// cleanSwapChain();

	// createSwapChain();
}

void GLWindow::cleanSwapChain() {}

void GLWindow::show() { SDL_ShowWindow(this->window); }

void GLWindow::hide() { SDL_HideWindow(this->window); }

void GLWindow::close() {
	this->hide();
	SDL_DestroyWindow(this->window);
}

void GLWindow::setPosition(int x, int y) { SDL_SetWindowPosition(this->window, x, y); }

void GLWindow::setSize(int width, int height) {
	/*	TODO determine if it shall update framebuffera as well.	*/
	SDL_SetWindowSize(this->window, width, height);
}

void GLWindow::vsync(bool state) {}

// GLWindow::~RendererWindow() {}

void GLWindow::getPosition(int *x, int *y) const { SDL_GetWindowPosition(this->window, x, y); }

void GLWindow::getSize(int *width, int *height) const { SDL_GetWindowSize(this->window, width, height); }

void GLWindow::setTitle(const char *title) { SDL_SetWindowTitle(this->window, title); }

std::string &GLWindow::getTitle() const { return SDLWindow::getTitle(); }
std::string GLWindow::getTitle() { return SDLWindow::getTitle(); }

void GLWindow::resizable(bool resizable) { SDL_SetWindowResizable(this->window, (SDL_bool)resizable); }

void GLWindow::setFullScreen(bool fullscreen) {
	// TODO add option for using either of the modes.
	if (fullscreen)
		SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		SDL_SetWindowFullscreen(this->window, 0);
}

bool GLWindow::isFullScreen() const {
	return SDLWindow::isFullScreen();
}

void GLWindow::setBordered(bool bordered) { SDL_SetWindowBordered(this->window, (SDL_bool)bordered); }

int GLWindow::width() const {
	int w, h;
	getSize(&w, &h);
	return w;
}
int GLWindow::height() const {
	int w, h;
	getSize(&w, &h);
	return h;
}

void GLWindow::setMinimumSize(int width, int height) { SDL_SetWindowMinimumSize(this->window, width, height); }
void GLWindow::getMinimumSize(int *width, int *height) {}

void GLWindow::setMaximumSize(int width, int height) { SDL_SetWindowMaximumSize(this->window, width, height); }
void GLWindow::getMaximumSize(int *width, int *height) {}

void GLWindow::focus() { SDL_SetWindowInputFocus(this->window); }

void GLWindow::restore() { SDL_RestoreWindow(this->window); }

void GLWindow::maximize() { SDL_MaximizeWindow(this->window); }

void GLWindow::minimize() { SDL_MinimizeWindow(this->window); }

intptr_t GLWindow::getNativePtr() const { return 0; }

void GLWindow::Initialize() {}

void GLWindow::Release() {}

void GLWindow::draw() {}

void GLWindow::onResize(int width, int height) { recreateSwapChain(); }

void GLWindow::run() {
	/*	*/
	this->Initialize();

	SDL_Event event = {};
	bool isAlive = true;
	bool visible = true;

	while (isAlive) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				goto finished;
				// return; /*  Exit.  */
			case SDL_KEYDOWN:
				break;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
				case SDL_WINDOWEVENT_CLOSE:
					return;
				// case SDL_WINDOWEVENT_SIZE_CHANGED:
				case SDL_WINDOWEVENT_RESIZED:
					recreateSwapChain();
					onResize(event.window.data1, event.window.data2);
				case SDL_WINDOWEVENT_HIDDEN:
				case SDL_WINDOWEVENT_MINIMIZED:
					visible = 0;
					break;
				case SDL_WINDOWEVENT_EXPOSED:
				case SDL_WINDOWEVENT_SHOWN:
					visible = 1;
					break;
				}
				break;

			default:
				break;
			}
		}
		if (visible) {
			this->draw();

			this->swapBuffer();
			this->getTimer().update();
			this->fpsCounter.incrementFPS(SDL_GetPerformanceCounter());

			std::cout << "FPS " << getFPSCounter().getFPS() << " Elapsed Time: " << getTimer().getElapsed()
					  << std::endl;
		}
	}
finished:
	return;
}
