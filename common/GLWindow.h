#ifndef _STARTUP_WINDOW_SAMPLE_H_
#define _STARTUP_WINDOW_SAMPLE_H_ 1
#include "SDLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <memory>
#include <vector>

class GLWindow : public SDLWindow {
  public:
	GLWindow(int x, int y, int width, int height);
	GLWindow(const GLWindow &other) = delete;
	~GLWindow(void);

  public:
	/**
	 * @brief
	 *
	 */
	virtual void Initialize(void);
	/**
	 * @brief
	 *
	 */
	virtual void Release(void);
	/**
	 * @brief
	 *
	 */
	virtual void draw(void);
	/**
	 * @brief
	 *
	 */
	virtual void run(void);

	/**
	 * @brief
	 *
	 */
	virtual void onResize(int width, int height);

  public:
	virtual void swapBuffer(void);

	virtual void close(void);

	virtual void show(void);

	virtual void hide(void);

	virtual void focus(void);

	virtual void restore(void);

	virtual void maximize(void);

	virtual void minimize(void);

	virtual void setPosition(int x, int y);

	virtual void setSize(int width, int height);

	virtual void vsync(bool state);

	virtual void getPosition(int *x, int *y) const;

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable);

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen(void) const;

	virtual void setBordered(bool bordered);

	virtual int width(void) const;
	virtual int height(void) const;

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);

	virtual void setTitle(const char *title);
	virtual std::string &getTitle(void) const;
	virtual std::string getTitle(void);

	virtual intptr_t getNativePtr(void) const;

  protected:
	virtual void createSwapChain(void);
	virtual void recreateSwapChain(void);
	virtual void cleanSwapChain(void);

  private:
	SDL_GLContext glcontext;
};

#endif
