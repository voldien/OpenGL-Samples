#pragma once
#include "MIMIWindow.h"
#include <IOUtil.h>
#include <cxxopts.hpp>

class GLSampleSession {
  public:
	virtual void run() {}
	virtual void commandline(cxxopts::Options &options) {}
};

template <class T> class GLSample : public GLSampleSession {
  public:
	GLSample(int argc, const char **argv) {

		/*	Parse argument.	*/
		const std::string helperInfo = "OpenGL Sample\n"
									   ""
									   "";

		/*	*/
		cxxopts::Options options("OpenGL Sample", helperInfo);
		options.add_options()("h,help", "helper information.")("d,debug", "Enable Debug View.",
															   cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"));

		auto result = options.parse(argc, (char **&)argv);
		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help();
			exit(EXIT_SUCCESS);
		}

		bool debug = result["debug"].as<bool>();
		if (result.count("time") > 0) {
		}

		this->ref = new T();
	}

	void run() { this->ref->run(); }

	void screenshot(float scale) {}

  private:
	T *ref;
};

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
