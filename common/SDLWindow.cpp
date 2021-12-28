#include"SDLWindow.h"

void SDLWindow::show() { SDL_ShowWindow(this->window); }

void SDLWindow::hide() { SDL_HideWindow(this->window); }

void SDLWindow::close() {
	this->hide();
	SDL_DestroyWindow(this->window);
}

void SDLWindow::setPosition(int x, int y) { SDL_SetWindowPosition(this->window, x, y); }

void SDLWindow::setSize(int width, int height) {
	/*	TODO determine if it shall update framebuffera as well.	*/
	SDL_SetWindowSize(this->window, width, height);
}

void SDLWindow::getPosition(int *x, int *y) const { SDL_GetWindowPosition(this->window, x, y); }

void SDLWindow::getSize(int *width, int *height) const { SDL_GetWindowSize(this->window, width, height); }
void SDLWindow::setTitle(std::string &title){}

std::string &SDLWindow::getTitle() const{

}
std::string SDLWindow::getTitle(){}

int SDLWindow::x() const {
	int x, y;
	SDL_GetWindowPosition(this->window, &x, &y);
	return x;
}
int SDLWindow::y() const {
	int x, y;
	SDL_GetWindowPosition(this->window, &x, &y);
	return y;
}
void SDLWindow::resizable(bool resizable) { SDL_SetWindowResizable(this->window, (SDL_bool)resizable); }

void SDLWindow::setFullScreen(bool fullscreen) {
	// TODO add option for using either of the modes.
	if (fullscreen)
		SDL_SetWindowFullscreen(this->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	else
		SDL_SetWindowFullscreen(this->window, 0);
}

bool SDLWindow::isFullScreen() const {}

void SDLWindow::setBordered(bool bordered) { SDL_SetWindowBordered(this->window, (SDL_bool)bordered); }

int SDLWindow::width() const {
	int w, h;
	getSize(&w, &h);
	return w;
}
int SDLWindow::height() const {
	int w, h;
	getSize(&w, &h);
	return h;
}

void SDLWindow::setMinimumSize(int width, int height) { SDL_SetWindowMinimumSize(this->window, width, height); }
void SDLWindow::getMinimumSize(int *width, int *height) {}

void SDLWindow::setMaximumSize(int width, int height) { SDL_SetWindowMaximumSize(this->window, width, height); }
void SDLWindow::getMaximumSize(int *width, int *height) {}

void SDLWindow::focus() { SDL_SetWindowInputFocus(this->window); }

void SDLWindow::restore() { SDL_RestoreWindow(this->window); }

void SDLWindow::maximize() { SDL_MaximizeWindow(this->window); }

void SDLWindow::minimize() { SDL_MinimizeWindow(this->window); }