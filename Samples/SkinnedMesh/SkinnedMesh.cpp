#include "UIComponent.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>

namespace glsample {

	class SkinnedMesh : public GLSampleWindow {

	  public:
		SkinnedMesh() : GLSampleWindow() {}
	};

	class SkinnedMeshGLSample : public GLSample<SkinnedMesh> {
	  public:
		SkinnedMeshGLSample() : GLSample<SkinnedMesh>() {}

		virtual void customOptions(cxxopts::OptionAdder &options) override {
			options("T,texture", "Texture Path", cxxopts::value<std::string>()->default_value("asset/texture.png"));
		}
	};
}; // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::SkinnedMeshGLSample sample;
		sample.run(argc, argv);
	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
