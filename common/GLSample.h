#pragma once
#include "GLSampleSession.h"
#include "IOUtil.h"
#include "Util/CameraController.h"
#include <GLHelper.h>
#include <GeometryUtil.h>
#include <ProceduralGeometry.h>
#include <SDLDisplay.h>
#include <cxxopts.hpp>

template <class T> class GLSample : public glsample::GLSampleSession {
  public:
	GLSample() {}

	~GLSample() { this->sampleRef->Release(); }

	virtual void run(int argc, const char **argv) override {

		/*	Parse argument.	*/
		const std::string helperInfo = "OpenGL Sample\n"
									   ""
									   "";

		/*	Default common options between all samples.	*/
		cxxopts::Options options("OpenGL Sample", helperInfo);
		cxxopts::OptionAdder &addr = options.add_options("OpenGL-Samples")("h,help", "helper information.")(
			"d,debug", "Enable Debug View.", cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"f,fullscreen", "Run in FullScreen Mode", cxxopts::value<bool>()->default_value("false"))(
			"v,vsync", "Vertical Blank Sync", cxxopts::value<bool>()->default_value("false"))(
			"g,opengl-version", "OpenGL Version", cxxopts::value<int>()->default_value("-1"))(
			"F,filesystem", "FileSystem", cxxopts::value<std::string>()->default_value("."));

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

		if (result.count("time") > 0) {
			/*	*/
			if (result["time"].as<float>() > 0) {
				int64_t timeout_mili = (int64_t)(result["time"].as<float>() * 1000.0f);
				std::thread timeout_thread = std::thread([&]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(timeout_mili));
					exit(EXIT_SUCCESS);
				});
				timeout_thread.detach();
			}
		}

		/*	Default window size.	*/
		int width = -1;
		int height = -1;

		/*	Create filesystem that the asset will be read from.	*/
		this->activeFileSystem = FileSystem::createFileSystem();
		std::string filesystemPath = result["filesystem"].as<std::string>();
		if (!this->activeFileSystem->isDirectory(filesystemPath.c_str())) {

			const std::string extension = this->activeFileSystem->getFileExtension(filesystemPath.c_str());
			if (extension == ".zip") {
				std::cout << "Found Zip File System: " << filesystemPath << std::endl;
				this->activeFileSystem = ZipFileSystem::createZipFileObject(filesystemPath.c_str());
			}
		}

		this->sampleRef = new T();
		this->sampleRef->setCommandResult(result);

		/*	Prevent residual errors to cause crash.	*/
		fragcore::resetErrorFlag();

		/*	Internal initialize.	*/
		this->sampleRef->setFileSystem(activeFileSystem);

		/*	Init the sample.	*/
		this->sampleRef->Initialize();

		/*	Set debugging state.	*/
		this->sampleRef->debug(debug);

		/*	*/
		if (fullscreen) {
			// Compute window size
			SDLDisplay display = SDLDisplay::getPrimaryDisplay();
			width = display.width();
			height = display.height();
		}
		this->sampleRef->setSize(width, height);
		// this->sampleRef->vsync(vsync);
		this->sampleRef->setFullScreen(fullscreen);

		this->sampleRef->show();
		this->sampleRef->run();
	}

  private:
	T *sampleRef;
};
