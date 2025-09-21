#pragma once
#include "FragDef.h"
#include "Scene/Scene.h"

namespace glsample {

	class FVDECLSPEC PBRScene : public Scene {
	  public:
		PBRScene() = default;

		void shadowPass();

	  public:
		void init(IFileSystem *filesystem) override;
		void bindMaterial(const Material *material) override;

		void render(Camera *camera, FrameBuffer *framebuffer = nullptr) override;
		void render(FrameBuffer *framebuffer = nullptr) override;

	  protected:
		bool UseShadowPass = false;

	  public:
		/*	Shadow shader paths.	*/

		/*	*/
		const std::string vertexMultiPassShaderPath = "Shaders/multipass/multipass.vert.spv";
		const std::string fragmentMultiPassShaderPath = "Shaders/multipass/multipass.frag.spv";

		// const std::string fragmentDirectionalClippingShadowShaderPath =
		// "Shaders/scene/shadow/scene_directional_shadow.vert.spv";

		unsigned int shadow_directional{};
		unsigned int shadow_directional_alpha{};

		unsigned int shadow_point{};
		unsigned int shadow_point_alpha{};

		using VariableRateSettings = struct variable_rate_shading_t {
			unsigned int variable_rate_color_program{};
			unsigned int variable_depth_edge_rate_program{};
			unsigned int variable_visual_edge_rate_program{};

			std::array<int, 3> localWorkGroupSize{};
			unsigned int variable_rate_lut_texture = 0;
			unsigned int variable_rate_visual_texture = 0;
		};
	};
} // namespace glsample
