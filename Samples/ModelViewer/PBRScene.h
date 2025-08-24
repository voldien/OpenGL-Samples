#pragma once
#include "FragDef.h"
#include "IO/FileSystem.h"
#include "Scene.h"

namespace glsample {

	class FVDECLSPEC PBRScene : public Scene {
	  public:
		PBRScene() = default;

		void shadowPass();
		void init(IFileSystem *filesystem);

	  public:
		void init() override;
		void bindMaterial(const MaterialObject *material) override;

		void render(Camera *camera) override;
		void render() override;

	  protected:
		bool UseShadowPass = false;

	  public:
		/*	Shadow shader paths.	*/

		//const std::string fragmentDirectionalClippingShadowShaderPath =  "Shaders/scene/shadow/scene_directional_shadow.vert.spv";

		unsigned int shadow_directional;
	};
} // namespace glsample
