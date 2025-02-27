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
#include "Common.h"
#include "GLRendererInterface.h"
#include "GLSampleSession.h"
#include "GLSampleWindow.h"
#include "GLUIComponent.h"
#include "IOUtil.h"
#include "IRenderer.h"
#include "TaskScheduler/IScheduler.h"
#include "Util/CameraController.h"
#include "Util/ProcessDataUtil.h"
#include <GLHelper.h>
#include <GeometryUtil.h>
#include <ProceduralGeometry.h>
#include <SDLDisplay.h>
#include <TaskScheduler.h>
#include <cxxopts.hpp>

/**
 * @brief
 *
 * @tparam T
 */
template <typename T = GLSampleWindow> class GLSample : public glsample::GLSampleSession {
  public:
	GLSample() noexcept = default;

	GLSample(const GLSample &) = delete;
	GLSample(GLSample &&) = delete;
	GLSample &operator=(const GLSample &) = delete;
	GLSample &operator=(GLSample &&) = delete;
	explicit GLSample(T *sampleRef) : sampleRef(sampleRef) {}
	~GLSample() override {
		this->sampleRef->Release();
		delete this->sampleRef;
		delete this->getFileSystem();
		delete this->getSchedular();
	}

	void run(int argc, const char **argv, const std::vector<const char *> &requiredExtension = {}) override {

		/*	Parse argument.	*/
		const std::string helperInfo = "OpenGL Sample: " + fragcore::SystemInfo::getApplicationName() + "\n";

		/*	Default common options between all samples.	*/
		cxxopts::Options options("OpenGL Sample: " + fragcore::SystemInfo::getApplicationName(), helperInfo);
		// TODO: default to debug during development and false during release
		cxxopts::OptionAdder &addr =
			options.add_options(fragcore::SystemInfo::getApplicationName())("h,help", "helper information.")(
				"d,debug", "Enable Debug View.", cxxopts::value<bool>()->default_value("true"))(
				"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
				"f,fullscreen", "Run in FullScreen Mode", cxxopts::value<bool>()->default_value("false"))(
				"v,vsync", "Vertical Blank Sync", cxxopts::value<bool>()->default_value("false"))(
				"g,opengl-version", "OpenGL Version", cxxopts::value<int>()->default_value("-1"))(
				"F,filesystem", "FileSystem", cxxopts::value<std::string>()->default_value("."))(
				"r,renderdoc", "Enable RenderDoc", cxxopts::value<bool>()->default_value("false"))(
				"G,gamma-correction", "Enable Gamma Correction", cxxopts::value<bool>()->default_value("false"))(
				"W,width", "Set Window Width", cxxopts::value<int>()->default_value("-1"))(
				"H,height", "Set Window Height", cxxopts::value<int>()->default_value("-1"))(
				"D,display", "Display", cxxopts::value<int>()->default_value("-1"))(
				"R,dynamic-range", "Set Dynamic Range ldr,hdr16,hdr32",
				cxxopts::value<std::string>()->default_value("hdr16"))("m,multi-sample", "Set MSAA",
																	   cxxopts::value<int>()->default_value("0"))(
				"p,use-postprocessing", "Use Post Processing", cxxopts::value<bool>()->default_value("true"))(
				"s,glsl-version", "Override glsl version from system (110,120,130,140,150,330...)",
				cxxopts::value<int>()->default_value("-1"));

		/*	Append command option for the specific sample.	*/
		this->customOptions(addr);

		/*	Parse the command line input.	*/
		options.allow_unrecognised_options();
		const auto result = options.parse(argc, (char **&)argv);

		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help(options.groups()) << std::endl;
			exit(EXIT_SUCCESS);
		}

		/*	*/
		const bool debug = result["debug"].as<bool>();
		const bool fullscreen = result["fullscreen"].as<bool>();
		const bool vsync = result["vsync"].as<bool>();
		const glsample::ColorSpace gammacorrection = glsample::ColorSpace::SRGB;

		/*	Default window size.	*/
		int window_x = 0, window_y = 0;
		int width = result["width"].as<int>();
		int height = result["height"].as<int>();
		const int msaa = result["multi-sample"].as<int>();
		const int display_index = result["display"].as<int>();

		fragcore::Ref<fragcore::IScheduler> schedular =
			fragcore::Ref<fragcore::IScheduler>(new fragcore::TaskScheduler(2));

		/*	Create filesystem that the asset will be read from.	*/
		this->activeFileSystem = fragcore::FileSystem::createFileSystem(schedular);
		const std::string filesystemPath = result["filesystem"].as<std::string>();
		if (!this->activeFileSystem->isDirectory(filesystemPath.c_str())) {

			const std::string extension = this->activeFileSystem->getFileExtension(filesystemPath.c_str());
			if (extension == ".zip") {
				std::cout << "Found Zip File System: " << filesystemPath << std::endl;
				this->activeFileSystem = fragcore::ZipFileSystem::createZipFileObject(filesystemPath.c_str());
			}
		}

		/*	*/
		this->sampleRef = new T();

		/*	Check if required extensions */
		bool all_required = true;
		if (requiredExtension.size() > 0) {

			const std::shared_ptr<fragcore::IRenderer> &renderer = this->sampleRef->getRenderInterface();

			const fragcore::GLRendererInterface &glRenderer = renderer->as<fragcore::GLRendererInterface>();

			/* Check all extension.	*/
			for (size_t i = 0; i < requiredExtension.size(); i++) {
				if (!glRenderer.isExtensionSupported(requiredExtension[i])) {
					this->sampleRef->getLogger().error("{} Not Supported", requiredExtension[i]);
					all_required = false;
				}
			}
		}

		/*	*/
		if (!all_required) {
			this->sampleRef->getLogger().info("Bye Bye ^^");
			delete this->sampleRef;
			return;
		}

		/*	Prevent residual errors to cause crash.	*/
		fragcore::resetErrorFlag();

		/*	Internal initialize.	*/
		this->sampleRef->setFileSystem(activeFileSystem);

		this->sampleRef->setCommandResult(result);

		if (msaa > 0) {
			/*	Set Default framebuffer multisampling.	*/
			glEnable(GL_MULTISAMPLE);
		}

		/*	Init the sample.	*/
		this->sampleRef->Initialize();

		/*	Make sure everything has been executed from init.	*/
		glFinish();

		/*	Set debugging state.	*/
		this->sampleRef->debug(debug);

		fragcore::SDLDisplay display = fragcore::SDLDisplay::getPrimaryDisplay();
		if (display_index >= 0) {
			display = fragcore::SDLDisplay::getDisplay(display_index);
		}

		/*	*/
		if (fullscreen) {

			/* Compute window size	*/
			width = display.width();
			height = display.height();

			window_x = display.x();
			window_y = display.y();
		} else if (width == -1 || height == -1) {

			/* Compute window size	*/
			width = display.width() / 2;
			height = display.height() / 2;

			window_x = display.x() + width;
			window_y = display.y() + height;
		}
		this->sampleRef->setPosition(window_x, window_y);
		this->sampleRef->setSize(width, height);

		this->sampleRef->vsync(vsync);
		this->sampleRef->setColorSpace(gammacorrection);
		this->sampleRef->setFullScreen(fullscreen);

		if (result.count("time") > 0) {
			/*	Create seperate thread that count down.	*/
			if (result["time"].as<float>() > 0) {

				const int64_t timeout_mili = (int64_t)(result["time"].as<float>() * 1000.0f);
				std::thread timeout_thread = std::thread([timeout_mili]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(timeout_mili));
					exit(EXIT_SUCCESS);
				});
				timeout_thread.detach();
			}
		}

		this->sampleRef->show();
		this->sampleRef->run();

		this->sampleRef->getLogger().info("Bye Bye ^^");
	}

  private:
	T *sampleRef = nullptr;
};
