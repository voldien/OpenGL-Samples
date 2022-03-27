#ifndef _GL_SAMPLE_WINDOW_H_
#define _GL_SAMPLE_WINDOW_H_ 1
#include "GLSample.h"
#include "IOUtil.h"
#include "MIMIWindow.h"

class GLSampleWindow : public MIMIIMGUI::MIMIWindow {
  public:
	GLSampleWindow() : MIMIIMGUI::MIMIWindow(MIMIIMGUI::MIMIWindow::GfxBackEnd::ImGUI_OpenGL) {}
	virtual ~GLSampleWindow() {}
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
	/**
	 * @brief
	 *
	 */

  protected:
	virtual void displayMenuBar() override;
	virtual void renderUI() override;
};
#endif
