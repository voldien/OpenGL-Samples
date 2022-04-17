#ifndef _GL_SAMPLE_H_
#define _GL_SAMPLE_H_ 1
#include "GLSampleSession.h"
#include "IOUtil.h"
#include "Util/CameraController.h"
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
			"t,time", "How long to run sample", cxxopts::value<float>()->default_value("0"));

		/*	*/
		this->commandline(options);

		auto result = options.parse(argc, (char **&)argv);
		/*	If mention help, Display help and exit!	*/
		if (result.count("help") > 0) {
			std::cout << options.help(options.groups());
			exit(EXIT_SUCCESS);
		}

		bool debug = result["debug"].as<bool>();
		/*	Enable debugging.	*/

		if (result.count("time") > 0) {
		}

		FileSystem::createFileSystem();

		this->ref = new T();
	}

	void run() { this->ref->run(); }

	void screenshot(float scale) {}

  private:
	T *ref;
};

#endif