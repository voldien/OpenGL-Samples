#include "GLWindow.h"
#include <cassert>
#include <stdexcept>
#include<GL/glew.h>
#include<fmt/format.h>

GLWindow::~GLWindow(void) { this->close(); }

GLWindow::GLWindow( int x, int y, int width, int height) {

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
	/*	*/
	this->Initialize();
}

void GLWindow::swapBuffer(void) { SDL_GL_SwapWindow(this->window); }

void GLWindow::createSwapChain(void) { glcontext = SDL_GL_CreateContext(this->window); }

void GLWindow::recreateSwapChain(void) {


	//cleanSwapChain();

	//createSwapChain();
}

void GLWindow::cleanSwapChain(void) {

}

void GLWindow::show() { SDL_ShowWindow(this->window); }

void GLWindow::hide() { SDL_HideWindow(this->window); }

void GLWindow::close(void) {
	this->hide();
	SDL_DestroyWindow(this->window);
}

void GLWindow::setPosition(int x, int y) { SDL_SetWindowPosition(this->window, x, y); }

void GLWindow::setSize(int width, int height) {
	/*	TODO determine if it shall update framebuffera as well.	*/
	SDL_SetWindowSize(this->window, width, height);
}

void GLWindow::vsync(bool state) {}

// GLWindow::~RendererWindow(void) {}

void GLWindow::getPosition(int *x, int *y) const { SDL_GetWindowPosition(this->window, x, y); }

void GLWindow::getSize(int *width, int *height) const { SDL_GetWindowSize(this->window, width, height); }

void GLWindow::setTitle(const char *title) { SDL_SetWindowTitle(this->window, title); }

std::string &GLWindow::getTitle(void) const{

}
std::string GLWindow::getTitle(void){

}

void GLWindow::resizable(bool resizable) { SDL_SetWindowResizable(this->window, (SDL_bool)resizable); }

void GLWindow::setFullScreen(bool fullscreen) {
	// TODO add option for using either of the modes.
	if (fullscreen)
		SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		SDL_SetWindowFullscreen(this->window, 0);
}

bool GLWindow::isFullScreen(void) const {}

void GLWindow::setBordered(bool bordered) { SDL_SetWindowBordered(this->window, (SDL_bool)bordered); }

int GLWindow::width(void) const {
	int w, h;
	getSize(&w, &h);
	return w;
}
int GLWindow::height(void) const {
	int w, h;
	getSize(&w, &h);
	return h;
}

void GLWindow::setMinimumSize(int width, int height) { SDL_SetWindowMinimumSize(this->window, width, height); }
void GLWindow::getMinimumSize(int *width, int *height) {}

void GLWindow::setMaximumSize(int width, int height) { SDL_SetWindowMaximumSize(this->window, width, height); }
void GLWindow::getMaximumSize(int *width, int *height) {}

void GLWindow::focus(void) { SDL_SetWindowInputFocus(this->window); }

void GLWindow::restore(void) { SDL_RestoreWindow(this->window); }

void GLWindow::maximize(void) { SDL_MaximizeWindow(this->window); }

void GLWindow::minimize(void) { SDL_MinimizeWindow(this->window); }

intptr_t GLWindow::getNativePtr(void) const {}

void GLWindow::Initialize(void) {}

void GLWindow::Release(void) {}

void GLWindow::draw(void) {}

void GLWindow::onResize(int width, int height) { recreateSwapChain(); }

void GLWindow::run(void) {
	/*	*/
	this->Initialize();

	SDL_Event event = {};
	bool isAlive = true;
	bool visible;

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
		this->draw();
		this->swapBuffer();
	}
finished:
	return;
	//	vkQueueWaitIdle(getDefaultGraphicQueue());
}
