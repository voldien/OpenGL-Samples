/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "FPSCounter.h"
#include "GLRendererInterface.h"
#include "PostProcessing/ColorSpaceConverter.h"
#include "PostProcessing/PostProcessingManager.h"
#include "SDLInput.h"
#include "SampleHelper.h"
#include "TaskScheduler/IScheduler.h"
#include <Core/Time.h>
#include <IO/IFileSystem.h>
#include <MIMIWindow.h>
#include <ProceduralGeometry.h>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

class FVDECLSPEC GLSampleWindow : public nekomimi::MIMIWindow {
  public:
	GLSampleWindow();
	GLSampleWindow &operator=(const GLSampleWindow &) = delete;
	GLSampleWindow &operator=(GLSampleWindow &&) = delete;
	GLSampleWindow(const GLSampleWindow &other) = delete;
	GLSampleWindow(GLSampleWindow &&other) = delete;
	~GLSampleWindow() override;

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
	// TODO: refractor name
	virtual void draw() = 0;
	virtual void postDraw() {}

	virtual void update() = 0;

  public:
	virtual void onResize(int width, int height) {}

	void setTitle(const std::string &title) override;

  public: /*	*/
	glsample::FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }
	const glsample::FPSCounter<float> &getFPSCounter() const noexcept { return this->fpsCounter; }

	const fragcore::Time &getTimer() const noexcept { return this->time; }
	fragcore::Time &getTimer() noexcept { return this->time; }

	size_t getFrameCount() const noexcept { return this->frameCount; }

	size_t getFrameBufferIndex() const noexcept { return this->frameBufferIndex; }
	size_t getFrameBufferCount() const noexcept { return this->getNumberFrameBuffers(); }

	bool isDebug() const noexcept;
	void debug(const bool enable);

	void captureScreenShot();

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }
	void setFileSystem(fragcore::IFileSystem *filesystem) noexcept { this->filesystem = filesystem; }

	fragcore::IScheduler *getSchedular() const noexcept { return *this->filesystem->getScheduler(); }

	unsigned int getShaderVersion() const;

	bool supportSPIRV() const;

	/*	*/
	const cxxopts::ParseResult &getResult() const noexcept { return this->parseResult; }
	void setCommandResult(const cxxopts::ParseResult &result) {
		this->parseResult = result;
		this->internalInit();
	}

	fragcore::Input &getInput() noexcept { return this->input; }
	const fragcore::Input &getInput() const noexcept { return this->input; }

	const fragcore::GLRendererInterface *getGLRenderInterface() const noexcept {
		return &this->getRenderInterface()->as<const fragcore::GLRendererInterface>();
	}
	fragcore::GLRendererInterface *getGLRenderInterface() noexcept {
		return &this->getRenderInterface()->as<fragcore::GLRendererInterface>();
	}

	void setColorSpace(const glsample::ColorSpace srgb);
	glsample::ColorSpace getColorSpace() const noexcept;

	glsample::ColorSpaceConverter *getColorSpaceConverter() noexcept { return this->colorSpace; }
	const glsample::ColorSpaceConverter *getColorSpaceConverter() const noexcept { return this->colorSpace; }

	void vsync(const bool enable_vsync);

	void enableRenderDoc(const bool status);
	bool isRenderDocEnabled();
	void captureDebugFrame() noexcept;

	spdlog::logger &getLogger() const noexcept { return *this->logger; }

  public:
	void createDefaultFrameBuffer();
	void updateDefaultFramebuffer();
	int getDefaultFramebuffer() const noexcept;
	glsample::FrameBuffer *getFrameBuffer() { return this->defaultFramebuffer; }
	glsample::PostProcessingManager *getPostProcessingManager() const noexcept { return this->postprocessingManager; }

	/*	*/
	size_t debug_prev_frame_sample_count = 0;
	size_t debug_prev_frame_primitive_count = 0;
	size_t debug_prev_frame_cs_invocation_count = 0;
	size_t debug_prev_frame_frag_invocation_count = 0;
	size_t nrPrimitives = 0, nrSamples = 0, time_elapsed = 0;
	size_t time_resolution = 1000 * 1000;

  protected:
	void displayMenuBar() override;
	void renderUI() override;

	void internalInit();

  private:
	cxxopts::ParseResult parseResult;
	glsample::FPSCounter<float> fpsCounter;
	fragcore::Time time;
	fragcore::SDLInput input;
	bool debugGL = true;

	glsample::PostProcessingManager *postprocessingManager = nullptr;
	glsample::ColorSpaceConverter *colorSpace = nullptr;

	/*	*/
	size_t frameCount = 0;
	size_t frameBufferIndex = 0;
	size_t frameBufferCount = 0;
	std::array<unsigned int, 10> queries;
	fragcore::IFileSystem *filesystem; /*	*/

	int preWidth = -1;
	int preHeight = -1;

	glsample::FrameBuffer *defaultFramebuffer = nullptr;
	glsample::FrameBuffer *MMSAFrameBuffer = nullptr;

  protected:
	spdlog::logger *logger = nullptr;
	void *rdoc_api = nullptr;
};
