#ifndef _GL_WINDOW_H_
#define _GL_WINDOW_H_ 1
#include "SDLWindow.h"
#include <GL/glew.h>
#include <SDL2/SDL_video.h>
#include <memory>
#include <vector>

class GLWindow : public SDLWindow {
  public:
	GLWindow(int x, int y, int width, int height);
	GLWindow(const GLWindow &other) = delete;
	~GLWindow();

  public:
	/**
	 * @brief
	 *
	 */
	virtual void Initialize();
	/**
	 * @brief
	 *
	 */
	virtual void Release();
	/**
	 * @brief
	 *
	 */
	virtual void draw();
	/**
	 * @brief
	 *
	 */
	virtual void run();

	/**
	 * @brief
	 *
	 */
	virtual void onResize(int width, int height);

  public:
	virtual void swapBuffer();

	virtual void close();

	virtual void show();

	virtual void hide();

	virtual void focus();

	virtual void restore();

	virtual void maximize();

	virtual void minimize();

	virtual void setPosition(int x, int y);

	virtual void setSize(int width, int height);

	virtual void vsync(bool state);

	virtual void getPosition(int *x, int *y) const;

	virtual void getSize(int *width, int *height) const;

	virtual void resizable(bool resizable);

	virtual void setFullScreen(bool fullscreen);

	virtual bool isFullScreen() const;

	virtual void setBordered(bool bordered);

	virtual int width() const;
	virtual int height() const;

	virtual void setMinimumSize(int width, int height);
	virtual void getMinimumSize(int *width, int *height);
	virtual void setMaximumSize(int width, int height);
	virtual void getMaximumSize(int *width, int *height);

	virtual void setTitle(const char *title);
	virtual std::string &getTitle() const;
	virtual std::string getTitle();

	virtual intptr_t getNativePtr() const;

  protected:
	virtual void createSwapChain();
	virtual void recreateSwapChain();
	virtual void cleanSwapChain();

  private:
	SDL_GLContext glcontext;
};

#endif
