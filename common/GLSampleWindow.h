#ifndef _GL_SAMPLE_WINDOW_H_
#define _GL_SAMPLE_WINDOW_H_ 1
#include "FPSCounter.h"
#include "GLSample.h"
#include "IOUtil.h"
#include "Util/Time.hpp"
#include <Core/IO/IFileSystem.h>
#include <MIMIWindow.h>
#include <ProceduralGeometry.h>
#include <cxxopts.hpp>

class FVDECLSPEC GLSampleWindow : public nekomimi::MIMIWindow {
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

	virtual void update() = 0;

	virtual void onResize(int width, int height) {}

	virtual void setTitle(const std::string &title) override;

  public:
	FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }
	const FPSCounter<float> &getFPSCounter() const noexcept { return this->fpsCounter; }

	const glsample::Time &getTimer() const noexcept { return this->time; }
	glsample::Time &getTimer() noexcept { return this->time; }
	size_t getFrameCount() noexcept { return this->frameCount; }

	void debug(bool enable);

	void captureScreenShot();

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }

	void setFileSystem(fragcore::IFileSystem *filesystem) { this->filesystem = filesystem; }

	unsigned int getShaderVersion() const;

	bool supportSPIRV() const;

	cxxopts::ParseResult &getResult() { return this->parseResult; }
	void setCommandResult(cxxopts::ParseResult &result) { this->parseResult = result; }

	// const fragcore::GLRendererInterface *getRenderInterface();

  protected:
	virtual void displayMenuBar() override;
	virtual void renderUI() override;

  private:
	cxxopts::ParseResult parseResult;
	FPSCounter<float> fpsCounter;
	glsample::Time time;
	size_t frameCount = 0;
	unsigned int queries[10];
	fragcore::IFileSystem *filesystem;

	int preWidth, preHeight;

  protected:
	// TODO Enable RenderDoc
};

#endif
