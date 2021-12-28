#ifndef _GLCOMMON_WINDOW_H_
#define _GLCOMMON_WINDOW_H_ 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <string>

/**
 * @brief
 *
 */
class SDLWindow {
  public:
	virtual void show();

	virtual void hide();

	virtual void close();

	virtual void focus();

	virtual void restore();

	virtual void maximize();

	virtual void minimize();

	virtual void setTitle(std::string &title);

	virtual std::string &getTitle() const;
	virtual std::string getTitle();

	virtual int x() const;
	virtual int y() const;

	virtual int width() const;
	virtual int height() const;

	virtual void getPosition(int *x, int *y) const;

	virtual void setPosition(int x, int y);

	virtual void setSize(int width, int height);

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable);

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen() const;

	virtual void setBordered(bool borded);

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);

  protected:
	SDL_Window *window;
};

#endif
