#ifndef _VKCOMMON_WINDOW_H_
#define _VKCOMMON_WINDOW_H_ 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <string>

class SDLWindow {
  public:
	virtual void show(void);

	virtual void hide(void);

	virtual void close(void);

	virtual void focus(void);

	virtual void restore(void);

	virtual void maximize(void);

	virtual void minimize(void);

	virtual void setTitle(std::string &title);

	virtual std::string &getTitle(void) const;
	virtual std::string getTitle(void);

	virtual int x(void) const;
	virtual int y(void) const;

	virtual int width(void) const;
	virtual int height(void) const;

	virtual void getPosition(int *x, int *y) const;

	virtual void setPosition(int x, int y);

	virtual void setSize(int width, int height);

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable);

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen(void) const;

	virtual void setBordered(bool borded);

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);


  protected:
	SDL_Window *window;
};

#endif
