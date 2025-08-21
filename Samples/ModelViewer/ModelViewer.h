#include "SampleHelper.h"
#include "Scene.h"
#include "Skybox.h"
#include "Util/CameraController.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>

namespace glsample {

	class PBRScene : public Scene {
	  public:
		PBRScene() = default;

		void init() override;
		void bindMaterial(const MaterialObject *material) override;

	  protected:
		bool shadowPass = false;

	  public:
		/*	Shadow shader paths.	*/
		// const std::string vertexShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.vert.spv";
		// const std::string geomtryShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.geom.spv";
		// const std::string fragmentShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.frag.spv";
		// const std::string fragmentShadowAlphaClipShaderPath =
		//	"Shaders/shadowpointlight/pointlightshadow_alphaclip.frag.spv";

		/*	*/
		//  const std::string vertexDirectionalShadowShaderPath = "Shaders/shadowmap/shadowmap.vert.spv";
		//  const std::string fragmentDirectionalShadowShaderPath = "Shaders/shadowmap/shadowmap.frag.spv";
		//  const std::string fragmentDirectionalClippingShadowShaderPath =
		//	"Shaders/shadowmap/shadowmap_alpha.frag.spv";
	};

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

		unsigned int reflection_texture{};

		/*	*/
		PBRScene scene;
		Skybox skybox;

		unsigned int irradiance_texture{};

		unsigned int physical_based_rendering_program{};
		unsigned int simple_physical_based_rendering_program{};
		unsigned int skybox_program{};

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		const size_t nrUniformBuffer = 3;
		size_t skyboxUniformSize = 0;

		const NodeObject *rootNode{};
		CameraController camera;

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.vert.spv";
		const std::string fragmentPBRShaderPath = "Shaders/pbr/simplephysicalbasedrendering.frag.spv";

		/*	Advanced.	*/
		const std::string vertexShaderPath = "Shaders/pbr/physicalbasedrendering.vert.spv";
		const std::string fragmentShaderPath = "Shaders/pbr/physicalbasedrendering.frag.spv";
		const std::string ControlShaderPath = "Shaders/pbr/physicalbasedrendering.tesc.spv";
		const std::string EvoluationShaderPath = "Shaders/pbr/physicalbasedrendering.tese.spv";

		class ModelViewerSettingComponent : public GLUIComponent<ModelViewer> {

		  public:
			ModelViewerSettingComponent(ModelViewer &sample) : GLUIComponent(sample) { this->setName("Model Viewer"); }
			void draw() override {

				// ImGui::TextUnformatted("Tessellation");
				// ImGui::DragFloat("Displacement", &this->uniform.tessellation.gDispFactor, 1, 0.0f, 100.0f);
				// ImGui::DragFloat("Levels", &this->uniform.tessellation.tessLevel, 1, 0.0f, 10.0f);

				// ImGui::TextUnformatted("Debugging");
				// ImGui::Checkbox("WireFrame", &this->showWireFrame);

				/*	*/
				this->getRefSample().scene.renderUI();
			}

			bool showWireFrame = false;

		  private:
			// struct uniform_buffer_block &uniform;
		};

		std::shared_ptr<ModelViewerSettingComponent> modelviewerSettingComponent;

		void Release() override;

		void Initialize() override;

		void onResize(int width, int height) override;

		void draw() override;
		void update() override;
	};

} // namespace glsample
