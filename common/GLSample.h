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
	GLSample() noexcept {
		// TODO set working directory to exec path.
	}

	~GLSample() { this->sampleRef->Release(); }

	virtual void run(int argc, const char **argv, const std::vector<const char *> &requiredExtension = {}) override {

		/*	Parse argument.	*/
		const std::string helperInfo = "OpenGL Sample: " + fragcore::SystemInfo::getApplicationName() +
									   "\n"
									   ""
									   "";

		/*	Default common options between all samples.	*/
		cxxopts::Options options("OpenGL Sample: " + fragcore::SystemInfo::getApplicationName(), helperInfo);
		cxxopts::OptionAdder &addr = options.add_options(fragcore::SystemInfo::getApplicationName())(
			"h,help", "helper information.")("d,debug", "Enable Debug View.",
											 cxxopts::value<bool>()->default_value("true"))( // TODO: change to false.
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
			"m,multi-sample", "Set MSAA", cxxopts::value<int>()->default_value("0"));

		/*	Append command option for the specific sample.	*/
		this->customOptions(addr);

		/*	Parse the command line input.	*/
		auto result = options.parse(argc, (char **&)argv);

		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help(options.groups()) << std::endl;
			exit(EXIT_SUCCESS);
		}

		/*	*/
		const bool debug = result["debug"].as<bool>();
		const bool fullscreen = result["fullscreen"].as<bool>();
		const bool vsync = result["vsync"].as<bool>();
		const bool gammacorrection = result["gamma-correction"].as<bool>();

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

		/*	Default window size.	*/
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
		this->sampleRef->setCommandResult(result);

		if (msaa > 0) {
			/*	Set Default framebuffer multisampling.	*/
			glEnable(GL_MULTISAMPLE);
		}

		/*	Prevent residual errors to cause crash.	*/
		fragcore::resetErrorFlag();

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
		/*	Internal initialize.	*/
		this->sampleRef->setFileSystem(activeFileSystem);

		/*	Init the sample.	*/
		this->sampleRef->Initialize();

		/*	Make sure everything has been executed from init.	*/
		glFinish();

		/*	Set debugging state.	*/
		this->sampleRef->debug(debug);

		/*	*/
		if (fullscreen) {
			/* Compute window size	*/
			fragcore::SDLDisplay display = fragcore::SDLDisplay::getPrimaryDisplay();
			if (display_index >= 0) {
				display = fragcore::SDLDisplay::getDisplay(display_index);
			}

			width = display.width();
			height = display.height();
		}
		/*	*/
		if (width == -1 || height == -1) {
			fragcore::SDLDisplay display = fragcore::SDLDisplay::getPrimaryDisplay();
			width = display.width() / 2;
			height = display.height() / 2;
		}
		this->sampleRef->setSize(width, height);
		this->sampleRef->vsync(vsync);
		this->sampleRef->setColorSpace(gammacorrection);
		this->sampleRef->setFullScreen(fullscreen);

		this->sampleRef->show();
		this->sampleRef->run();
	}

  private:
	T *sampleRef;
};
