#ifndef _GL_SAMPLE_WINDOW_H_
#define _GL_SAMPLE_WINDOW_H_ 1
#include "FPSCounter.h"
#include "GLSample.h"
#include "IOUtil.h"
#include "Util/Time.hpp"
#include <MIMIWindow.h>

class FVDECLSPEC GLSampleWindow : public MIMIIMGUI::MIMIWindow {
  public:
	GLSampleWindow();

	/**
	 * @brief
	 *
	 */
	virtual void Initialize() = 0;
	/**
	 * @brief
	 *
	 */
	virtual void Release() = 0;
	/**
	 * @brief
	 *
	 */
	virtual void draw() = 0;

  public:
	FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }
	vkscommon::Time &getTimer() noexcept { return this->time; }
	size_t getFrameCount() noexcept { return this->frameCount; }

  protected:
	virtual void displayMenuBar() override;
	virtual void renderUI() override;

  private:
	FPSCounter<float> fpsCounter;
	vkscommon::Time time;
	size_t frameCount = 0;

  protected:
	// TODO Enable RenderDoc
};

#endif
