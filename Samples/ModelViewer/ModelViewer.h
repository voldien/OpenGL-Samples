#include "SampleHelper.h"
#include "Scene.h"
#include "Skybox.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>

namespace glsample {

	/**
	 * @brief
	 */
	class ModelViewer : public GLSampleWindow {
	  public:
		ModelViewer();

		struct tessellation_settings {
			float tessLevel = 1;
			float gDispFactor = 1;
		};

		struct uniform_buffer_block {
			glm::mat4 model{};
			glm::mat4 view{};
			glm::mat4 proj{};
			glm::mat4 modelView{};
			glm::mat4 viewProjection{};
			glm::mat4 modelViewProjection{};

			struct tessellation_settings tessellation;

		} uniformStageBuffer;

		unsigned int physical_based_rendering_program;
		unsigned int skybox_program;
		unsigned int irradiance_texture;

		Skybox skybox;
		Scene scene;
		CameraController cameraController;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.vert.spv";
		const std::string fragmentPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.frag.spv";

		/*	Advanced.	*/
		const std::string PBRvertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert.spv";
		const std::string PBRfragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag.spv";
		const std::string PBRControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc.spv";
		const std::string PBREvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese.spv";

		class ModelViewerSettingComponent : public GLUIComponent<ModelViewer> {

		  public:
			ModelViewerSettingComponent(ModelViewer &sample)
				: GLUIComponent(sample), uniform(sample.uniformStageBuffer) {
				this->setName("Model Viewer");
			}
			void draw() override {

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Displacement", &this->uniform.tessellation.gDispFactor, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Levels", &this->uniform.tessellation.tessLevel, 1, 0.0f, 10.0f);

				ImGui::TextUnformatted("Debugging");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);

				/*	*/
				this->getRefSample().scene.renderUI();
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
		};

		std::shared_ptr<ModelViewerSettingComponent> modelviewerSettingComponent;

		void Release() override;

		void Initialize() override;

		void draw() override;
		void update() override;
	};

} // namespace glsample
