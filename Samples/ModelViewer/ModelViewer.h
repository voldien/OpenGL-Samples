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

		struct light_settings {

			DirectionalLight directionalLight;
			/*	*/
			PointLightInstance pointLights[4];
		};

		struct camera_settings {
			glm::vec4 gEyeWorldPos;
		};

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

			struct light_settings lightsettings;
			struct tessellation_settings tessellation;
			/*	Camera settings.	*/
			struct camera_settings camera {};

		} uniformStageBuffer;

		int physical_based_rendering_program;
		int skybox_program;

		Skybox skybox;
		Scene scene;
		CameraController camera;

		/*	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignBufferSize = sizeof(uniform_buffer_block);

		/*	Advanced.	*/
		const std::string PBRvertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert.spv";
		const std::string PBRfragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag.spv";
		const std::string PBRControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc.spv";
		const std::string PBREvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese.spv";

		class ModelViewerSettingComponent : public nekomimi::UIComponent {

		  public:
			ModelViewerSettingComponent(struct uniform_buffer_block &uniform) : uniform(uniform) {
				this->setName("Model Viewer");
			}
			void draw() override {

				//
				// ImGui::ColorEdit4("Light", &this->uniform.lightColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::ColorEdit4("Ambient", &this->uniform.ambientColor[0],
				//				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat3("Direction", &this->uniform.direction[0]);

				ImGui::TextUnformatted("Light Settings");
				for (size_t i = 0;
					 i < sizeof(uniform.lightsettings.pointLights) / sizeof(uniform.lightsettings.pointLights[0]);
					 i++) {
					ImGui::PushID(1000 + i);
					if (ImGui::CollapsingHeader(fmt::format("Light {}", i).c_str(), &lightvisible[i],
												ImGuiTreeNodeFlags_CollapsingHeader)) {

						ImGui::ColorEdit4("Color", &this->uniform.lightsettings.pointLights[i].color[0],
										  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
						ImGui::DragFloat3("Position", &this->uniform.lightsettings.pointLights[i].position[0]);
						ImGui::DragFloat3("Attenuation",
										  &this->uniform.lightsettings.pointLights[i].constant_attenuation);
						ImGui::DragFloat("Range", &this->uniform.lightsettings.pointLights[i].range);
						ImGui::DragFloat("Intensity", &this->uniform.lightsettings.pointLights[i].intensity);
					}
					ImGui::PopID();
				}

				ImGui::TextUnformatted("Light");
				// ImGui::ColorEdit4("Color", &this->uniform.lightsettings.lightColor[0],
				// 				  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
				// ImGui::ColorEdit4("Ambient Color", &this->uniform.lightsettings.ambientColor[0],
				// 				  ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
				// ImGui::DragFloat3("Direction", &this->uniform.lightsettings.direction[0]);

				ImGui::TextUnformatted("Tessellation");
				ImGui::DragFloat("Displacement", &this->uniform.tessellation.gDispFactor, 1, 0.0f, 100.0f);
				ImGui::DragFloat("Levels", &this->uniform.tessellation.tessLevel, 1, 0.0f, 10.0f);

				ImGui::TextUnformatted("Debugging");
				ImGui::Checkbox("WireFrame", &this->showWireFrame);
			}

			bool showWireFrame = false;

		  private:
			struct uniform_buffer_block &uniform;
			bool lightvisible[4] = {true, true, true, true};
		};

		std::shared_ptr<ModelViewerSettingComponent> modelviewerSettingComponent;

		void Release() override;

		void Initialize() override;

		void draw() override;
		void update() override;
	};

} // namespace glsample
