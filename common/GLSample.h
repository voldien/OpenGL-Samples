#ifndef _GL_SAMPLE_H_
#define _GL_SAMPLE_H_ 1
#include "GLSampleSession.h"
#include "IOUtil.h"
#include "Util/CameraController.h"
#include <GLHelper.h>
#include <GeometryUtil.h>
#include <ProceduralGeometry.h>
#include <SDLDisplay.h>
#include <cxxopts.hpp>

template <class T> class GLSample : public GLSampleSession {
  public:
	GLSample(int argc, const char **argv) {

		/*	Parse argument.	*/
		const std::string helperInfo = "OpenGL Sample\n"
									   ""
									   "";

		/*	Default common options between all samples.	*/
		cxxopts::Options options("OpenGL Sample", helperInfo);
		options.add_options("OpenGL-Samples")("h,help", "helper information.")(
			"d,debug", "Enable Debug View.", cxxopts::value<bool>()->default_value("true"))(
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"))(
			"f,fullscreen", "Run in FullScreen Mode", cxxopts::value<bool>()->default_value("false"))(
			"v,vsync", "Vertical Blank Sync", cxxopts::value<bool>()->default_value("false"))(
			"g,opengl-version", "OpenGL Version", cxxopts::value<bool>()->default_value("false"))(
			"F,filesystem", "FileSystem", cxxopts::value<std::string>()->default_value("."));

		/*	Append command option for the specific sample.	*/
		this->commandline(options);

		/*	Parse the command line input.	*/
		auto result = options.parse(argc, (char **&)argv);

		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help(options.groups());
			exit(EXIT_SUCCESS);
		}

		/*	*/
		const bool debug = result["debug"].as<bool>();
		const bool fullscreen = result["fullscreen"].as<bool>();
		const bool vsync = result["vsync"].as<bool>();

		if (result.count("time") > 0) {
		}

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
	}

	~GLSample() { this->sampleRef->Release(); }

	void run() {
		this->sampleRef->show();
		this->sampleRef->run();
	}

	void screenshot(float scale) {}

  private:
	T *sampleRef;
};

#endif