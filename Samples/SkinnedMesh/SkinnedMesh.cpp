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

	/**
	 * @brief
	 *
	 */
	class SkinnedMesh : public GLSampleWindow {

	  public:
		SkinnedMesh() : GLSampleWindow() {
			this->setTitle("Skinned Mesh");
			this->spotLightSettingComponent = std::make_shared<SpotLightSettingComponent>(this->uniformStageBuffer);
			this->addUIComponent(this->spotLightSettingComponent);

			this->camera.setPosition(glm::vec3(18.5f));
			this->camera.lookAt(glm::vec3(0.f));
		}

		struct UniformBufferBlock {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::mat4 modelView;
			glm::mat4 modelViewProjection;

			/*	light source.	*/
			glm::vec4 ambientLight = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);

		} uniformStageBuffer;

		CameraController camera;

		class SpotLightSettingComponent : public nekomimi::UIComponent {

		  public:
			SpotLightSettingComponent(struct UniformBufferBlock &uniform) : uniform(uniform) {
				this->setName("Skinned Mesh");
			}
			void draw() override {}

		  private:
			struct UniformBufferBlock &uniform;
			bool lightvisible[4] = {true, true, true, true};
		};
		std::shared_ptr<SpotLightSettingComponent> spotLightSettingComponent;

		void Release() override {}
		void Initialize() override {}
		void draw() override {}
		void update() override {}
	};

	class SkinnedMeshGLSample : public GLSample<SkinnedMesh> {
	  public:
		SkinnedMeshGLSample() : GLSample<SkinnedMesh>() {}

		void customOptions(cxxopts::OptionAdder &options) override {
			options("M,model", "Model Path", cxxopts::value<std::string>()->default_value("asset/sponza/sponza.obj"));
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
