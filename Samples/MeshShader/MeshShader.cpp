#include "Exception.hpp"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ImageImport.h>
#include <ImportHelper.h>
#include <ModelImporter.h>
#include <ShaderLoader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace glsample {
	class MeshShader : public GLSampleWindow {
	  public:
		void Release() override {}

		void Initialize() override {

			int x = 0;
			glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &x);
			this->getLogger().info("Max Mesh Output Vertices {0}", x);
			glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &x);
			this->getLogger().info("Max Mesh Output Primitives {0}", x);
		}

		void draw() override {}
		void update() override {}
	};

	class MeshShaderGLSample : public GLSample<MeshShader> {
	  public:
		MeshShaderGLSample() : GLSample<MeshShader>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"))(
				"T,skybox", "Skybox Texture Path",
				cxxopts::value<std::string>()->default_value("asset/snowy_forest_4k.exr"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {

	const std::vector<const char *> required_extensions = {"GL_NV_mesh_shader"};

	try {
		glsample::MeshShaderGLSample sample;
		sample.run(argc, argv, required_extensions);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}