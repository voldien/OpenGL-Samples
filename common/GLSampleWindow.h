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
#include "Math3D/Math3D.h"
#include "PostProcessing/ColorSpaceConverter.h"
#include "PostProcessing/PostProcessingManager.h"
#include "SDLInput.h"
#include "SampleHelper.h"
#include "TaskScheduler/IScheduler.h"
#include <Core/Time.h>
#include <IO/IFileSystem.h>
#include <MIMIWindow.h>
#include <Math3D/Math3D.h>
#include <ProceduralGeometry.h>
#include <cstddef>
#include <cxxopts.hpp>
#include <memory>
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

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem.get(); }
	void setFileSystem(fragcore::IFileSystem *filesystem) noexcept {
		this->filesystem = std::shared_ptr<fragcore::IFileSystem>(filesystem);
	}

	fragcore::IScheduler *getSchedular() const noexcept { return this->filesystem->getScheduler().get(); }

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

	std::shared_ptr<glsample::ColorSpaceConverter> &getColorSpaceConverter() noexcept { return this->colorSpace; }
	const std::shared_ptr<glsample::ColorSpaceConverter> &getColorSpaceConverter() const noexcept {
		return this->colorSpace;
	}

	void vsync(const bool enable_vsync);
	bool getVSync() const;

	void enableRenderDoc(const bool status);
	void launchRenderDoc();
	bool isRenderDocEnabled() noexcept;
	void captureDebugFrame() noexcept;

	spdlog::logger &getLogger() const noexcept { return *this->logger; }

  public:
	void createDefaultFrameBuffer();
	void updateDefaultFramebuffer();
	int getDefaultFramebuffer() const noexcept;
	glsample::FrameBuffer *getFrameBuffer() { return this->defaultFramebuffer.get(); }
	glsample::PostProcessingManager *getPostProcessingManager() const noexcept {
		return this->postprocessingManager.get();
	}

	// TODO: relocate
	static void blitFrameBuffer(const glsample::FrameBuffer *framebuffer, const unsigned int width, const int height,
								glm::vec4 rectNormalized, int mode = 0) {
									
		/*	Blit image targets to screen.	*/
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->framebuffer);
		/*	Transfer each target to default framebuffer.	*/
		const size_t widthDivior = 3;
		const size_t heightDivior = 2;

		const float sub_view_width = (int)(width / widthDivior);
		const float sub_view_height = (int)(height / heightDivior);

		// TODO: make its function for resue it with other samples
		for (size_t index = 0; index < framebuffer->nrAttachments; index++) {

			glReadBuffer(GL_COLOR_ATTACHMENT0 + index);

			/*	*/
			const size_t dest_width = sub_view_width + (index % widthDivior) * sub_view_width;
			const size_t dest_height = sub_view_height + (index / heightDivior) * sub_view_height;

			const size_t source_width = 0;
			const size_t source_height = 0;

			glBlitFramebuffer(0, 0, source_width, source_height, (index % widthDivior) * (sub_view_width),
							  (index / heightDivior) * sub_view_height, dest_width, dest_height, GL_COLOR_BUFFER_BIT,
							  GL_LINEAR);
		}
		// glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
	}

	/*	*/
	struct debug_info_t {
		size_t debug_prev_frame_sample_count = 0;
		size_t debug_prev_frame_primitive_count = 0;
		size_t debug_prev_frame_cs_invocation_count = 0;
		size_t debug_prev_frame_frag_invocation_count = 0;
		size_t debug_prev_frame_vertex_invocation_count = 0;
		size_t debug_prev_frame_geometry_invocation_count = 0;

		size_t nrPrimitives = 0, nrSamples = 0, time_elapsed = 0;
		size_t time_resolution = static_cast<long>(1000) * 1000;
	};

	size_t debug_prev_frame_sample_count = 0;
	size_t debug_prev_frame_primitive_count = 0;
	size_t debug_prev_frame_cs_invocation_count = 0;
	size_t debug_prev_frame_frag_invocation_count = 0;
	size_t debug_prev_frame_vertex_invocation_count = 0;
	size_t debug_prev_frame_geometry_invocation_count = 0;

	size_t nrPrimitives = 0, nrSamples = 0, time_elapsed = 0;
	size_t time_resolution = static_cast<long>(1000) * 1000;

  protected:
	void displayMenuBar() override;
	void renderUI() override;

	void internalInit();

  private:
	cxxopts::ParseResult parseResult;
	glsample::FPSCounter<float> fpsCounter;
	fragcore::Time time;
	fragcore::SDLInput input;

	std::shared_ptr<fragcore::IFileSystem> filesystem; /*	*/

	std::shared_ptr<glsample::PostProcessingManager> postprocessingManager = nullptr;
	std::shared_ptr<glsample::ColorSpaceConverter> colorSpace;

	std::shared_ptr<glsample::FrameBuffer> defaultFramebuffer = nullptr;
	std::shared_ptr<glsample::FrameBuffer> MMSAFrameBuffer = nullptr;

	/*	*/
	bool debugGL = true;
	size_t frameCount = 0;
	size_t frameBufferIndex = 0;
	size_t frameBufferCount = 0;
	std::array<unsigned int, 10> queries;

	int preWidth = -1;
	int preHeight = -1;

  protected:
	std::shared_ptr<spdlog::logger> logger;
	void *rdoc_api = nullptr;
};
