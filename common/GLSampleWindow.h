/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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
#include "ColorSpaceConverter.h"
#include "FPSCounter.h"
#include "GLRendererInterface.h"
#include "SDLInput.h"
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
	GLSampleWindow(const GLSampleWindow &other) = delete;
	GLSampleWindow(GLSampleWindow &&other) = delete;

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

	void debug(const bool enable);

	void captureScreenShot();

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }
	void setFileSystem(fragcore::IFileSystem *filesystem) noexcept { this->filesystem = filesystem; }

	fragcore::IScheduler *getSchedular() const noexcept { return *this->filesystem->getScheduler(); }

	unsigned int getShaderVersion() const;

	bool supportSPIRV() const;

	/*	*/
	cxxopts::ParseResult &getResult() noexcept { return this->parseResult; }
	void setCommandResult(cxxopts::ParseResult &result) noexcept { this->parseResult = result; }

	fragcore::SDLInput &getInput() noexcept { return this->input; }
	const fragcore::SDLInput &getInput() const noexcept { return this->input; }

	const fragcore::GLRendererInterface *getGLRenderInterface() const noexcept {
		return &this->getRenderInterface()->as<const fragcore::GLRendererInterface>();
	}
	fragcore::GLRendererInterface *getGLRenderInterface() noexcept {
		return &this->getRenderInterface()->as<fragcore::GLRendererInterface>();
	}

	void setColorSpace(bool srgb);
	void vsync(bool enable_vsync);

	void enableRenderDoc(bool status);
	bool isRenderDocEnabled();
	void captureDebugFrame() noexcept;

	spdlog::logger &getLogger() const noexcept { return *this->logger; }

  public:
	void createDefaultFrameBuffer();
	void updateDefaultFramebuffer();
	int getDefaultFramebuffer() const noexcept;

	size_t prev_frame_sample_count = 0;
	size_t prev_frame_primitive_count = 0;

  protected:
	void displayMenuBar() override;
	void renderUI() override; // TODO: rename

  private:
	cxxopts::ParseResult parseResult;
	glsample::FPSCounter<float> fpsCounter;
	fragcore::Time time;
	fragcore::SDLInput input;
	bool debugGL = true;

	glsample::ColorSpaceConverter *colorSpace;

	/*	*/
	size_t frameCount = 0;
	size_t frameBufferIndex = 0;
	size_t frameBufferCount = 0;
	unsigned int queries[10];
	fragcore::IFileSystem *filesystem;

	int preWidth, preHeight;

	/*	Default framebuffer.	*/
	using FrameBuffer = struct framebuffer_t {
		unsigned int framebuffer;
		unsigned int attachement0;
		unsigned int depthbuffer;
	};
	FrameBuffer *defaultFramebuffer = nullptr;

  protected:
	spdlog::logger *logger;
	void *rdoc_api = nullptr;
};
