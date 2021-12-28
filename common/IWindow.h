#ifndef _VKSCOMMON_IWINDOW_H_
#define _VKSCOMMON_IWINDOW_H_ 1
#include <VulkanCore.h>
#include <stdint.h>
#include <string>
/**
 * @brief
 *
 */
class IWindow {
  public:
	virtual void show() = 0;

	virtual void hide() = 0;

	virtual void close() = 0;

	virtual void focus() = 0;

	virtual void restore() = 0;

	virtual void maximize() = 0;

	virtual void minimize() = 0;

	virtual void setTitle(const std::string &title) = 0;

	virtual std::string getTitle() const = 0;

	virtual void getPosition(int *x, int *y) const = 0;

	virtual void setPosition(int x, int y) = 0;

	virtual void setSize(int width, int height) = 0;

	virtual void getSize(int *width, int *height) const = 0;
	virtual int x() const noexcept = 0;
	virtual int y() const noexcept = 0;

	virtual int width() const noexcept = 0;
	virtual int height() const noexcept = 0;

	virtual float getGamma() const = 0;

	virtual void setGamma(float gamma) = 0;

	virtual void resizable(bool resizable) = 0;

	virtual void setFullScreen(bool fullscreen) = 0;

	virtual bool isFullScreen() const = 0;

	virtual void setBordered(bool borded) = 0;

	virtual void setMinimumSize(int width, int height) = 0;
	virtual void getMinimumSize(int *width, int *height) = 0;
	virtual void setMaximumSize(int width, int height) = 0;
	virtual void getMaximumSize(int *width, int *height) = 0;

	void setUserData(void *userData) noexcept { this->userData = userData; }

	virtual void *getUserData() const noexcept { return this->userData; }

	virtual intptr_t getNativePtr() const = 0; /*  Get native window reference object. */


	virtual VkSurfaceKHR createSurface(std::shared_ptr<VulkanCore> &instance) = 0;

  protected:
	void *userData = nullptr;
};
#endif
