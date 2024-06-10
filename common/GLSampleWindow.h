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
#include "FPSCounter.h"
#include "SDLInput.h"
#include <Core/IO/IFileSystem.h>
#include <Core/Time.h>
#include <MIMIWindow.h>
#include <ProceduralGeometry.h>
#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

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

  public:
	virtual void onResize(int width, int height) {}

	virtual void setTitle(const std::string &title) override;

  public: /*	*/
	glsample::FPSCounter<float> &getFPSCounter() noexcept { return this->fpsCounter; }
	const glsample::FPSCounter<float> &getFPSCounter() const noexcept { return this->fpsCounter; }

	const fragcore::Time &getTimer() const noexcept { return this->time; }
	fragcore::Time &getTimer() noexcept { return this->time; }

	size_t getFrameCount() const noexcept { return this->frameCount; }

	size_t getFrameBufferIndex() const noexcept { return this->frameBufferIndex; }
	size_t getFrameBufferCount() const noexcept { return this->getNumberFrameBuffers(); }

	void debug(bool enable);

	void captureScreenShot();

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem; }
	void setFileSystem(fragcore::IFileSystem *filesystem) noexcept { this->filesystem = filesystem; }

	unsigned int getShaderVersion() const;

	bool supportSPIRV() const;

	/*	*/
	cxxopts::ParseResult &getResult() noexcept { return this->parseResult; }
	void setCommandResult(cxxopts::ParseResult &result) noexcept { this->parseResult = result; }

	fragcore::SDLInput &getInput() noexcept { return this->input; }
	const fragcore::SDLInput &getInput()const  noexcept { return this->input; }

	// const fragcore::GLRendererInterface *getRenderInterface();

	void setColorSpace(bool srgb);

	void enableRenderDoc(bool status);

	spdlog::logger &getLogger() const noexcept { return *this->logger; }

  protected:
	virtual void displayMenuBar() override;
	virtual void renderUI() override; // TODO: rename

  private:
	cxxopts::ParseResult parseResult;
	glsample::FPSCounter<float> fpsCounter;
	fragcore::Time time;
	fragcore::SDLInput input;
	bool debugGL = true;

	/*	*/
	size_t frameCount = 0;
	size_t frameBufferIndex = 0;
	size_t frameBufferCount = 0;
	unsigned int queries[10];
	fragcore::IFileSystem *filesystem;

	int preWidth, preHeight;

  protected:
	spdlog::logger *logger;
	// TODO Enable RenderDoc
};
