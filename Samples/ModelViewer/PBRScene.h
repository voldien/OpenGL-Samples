#pragma once
#include "FragDef.h"
#include "Scene.h"


namespace glsample {

	class FVDECLSPEC PBRScene : public Scene {
	  public:
		PBRScene() = default;


		void shadowPass();

		public:

		void init() override;
		void bindMaterial(const MaterialObject *material) override;

		void render(Camera *camera) override;
		void render() override;

	  protected:
		bool UseShadowPass = false;

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
} // namespace glsample
