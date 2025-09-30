#pragma once
#include "Math3D/LinAlg.h"
#include "RenderDesc.h"
#include "SampleHelper.h"
#include "Scene/CameraController.h"
#include "Scene/RenderQueue.h"
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

namespace glsample {

	enum class FogType : unsigned int {
		None,	/*	*/
		Linear, /*	*/
		Exp,	/*	*/
		Exp2,	/*	*/
		Height	/*	*/
	};

	using GammaCorrectionSettings = struct gamme_correct_settings_t {
		float exposure = 1.0f;
		float gamma = 2.2f;
	};

	using FogSettings = struct fog_settings_t {
		glm::vec4 fogColor = glm::vec4(0.45, 0.45, 0.45, 1);
		/*	*/
		float cameraNear = 0.15f;
		float cameraFar = 1000.0f;
		float fogStart = 100;
		float fogEnd = 1000;

		/*	*/
		float fogDensity = 0.5f;
		FogType fogType = FogType::Exp;
		float fogIntensity = 1.0f;
		float fogHeight = 0;
	};

	// TODO: relocate
	using GraphicShaderSettings = struct graphic_shader_settings_t { // Property Maybe ?
		fragcore::BlendEqu blend_equ = fragcore::BlendEqu::NoEqu;	 /*	aiBlendMode*/
		fragcore::BlendFunc blend_color_func = fragcore::BlendFunc::One;
		CullingMode cullingMode = CullingMode::Back;
		DepthFunc DepthFunc = DepthFunc::Less;
		bool DepthWrite{};
		RenderQueue queue;

		int wireframe_mode = 0; // TODO; change to fill mode

		bool culling_both_side_mode = false;
		float clipping = 1;
	};

	using TessellationSettings = struct tessellation_settings_t {
		float tessLevel = 1;
		float gDispFactor = 1;
	};

	using MaterialInstanceData = struct material_instance_t {
		glm::mat4 model;
		/*	Color attributes.	*/
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 emission;
		glm::vec4 specular;
		glm::vec4 transparent;
		glm::vec4 reflectivity;

		/*	*/
	};

	using BoundingShapeData = struct bounding_data_t {
		fragcore::Bound bound;
	};

	using BlinnPhongMaterialData = struct blinn_phong_material_data_t {};

	using LightDirectionalShadowData = struct light_shadow_t {
		glm::mat4 lightSpaceMatrix;
		glm::vec4 shadow; /*	Shadow, Bias,	*/
	};

	using LightPointShadowData = struct light_point_shadow_data_t {
		glm::mat4 lightSpaceMatrix[6];
		glm::vec4 shadow; /*	Shadow, Bias,	*/
	};

	using DirectionalLightData = struct directional_light_t {
		LightDirectionalShadowData lightShadow;
		glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
		glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	};

	using PointLightInstance = struct point_light_instance_t {
		LightDirectionalShadowData lightShadow;
		glm::vec3 position = glm::vec3(0);
		float range = 5;
		glm::vec4 color = glm::vec4(1);
		/*	*/
		float intensity = 1; // TODO: remove
		float constant_attenuation = 1;
		float linear_attenuation = 0.1f;
		float quadratic_attenuation = 0.025f;
	};

	using CameraInstanceData = struct camera_instance_data_t {

		/*	*/
		camera_instance_data_t &operator=(const Camera &camera) {

			this->position = glm::vec4(camera.getPosition(), 1);
			this->viewDir = glm::vec4(camera.forward(), 1);
			this->view = camera.getViewMatrix();
			this->proj = camera.getProjectionMatrix();
			this->viewProj = this->proj * this->view;
			this->viewInv = glm::inverse(camera.getViewMatrix());
			this->inverseProj = glm::inverse(camera.getProjectionMatrix());
			this->viewProjInv = glm::inverse(this->viewProj);

			this->near = camera.getNear();
			this->far = camera.getFar();

			return *this;
		}

		camera_instance_data_t &operator=(CameraController &camera) {
			//*this = camera.as<Camera<float>>();

			// TODO: reuse function above
			this->near = camera.getNear();
			this->far = camera.getFar();
			this->proj = camera.getProjectionMatrix();
			this->inverseProj = glm::inverse(this->proj);
			this->position = glm::vec4(camera.getPosition(), 0);

			this->near = camera.getNear();
			this->far = camera.getFar();
			this->viewDir = glm::vec4(camera.getLookDirection(), 0);
			this->view = camera.getViewMatrix();
			this->viewInv = glm::inverse(this->view);

			this->viewRot = camera.getRotationMatrix();
			this->viewProj = this->proj * this->view;
			this->viewProjInv = glm::inverse(this->viewProj);
			return *this;
		}

		/*	*/
		float near = 0.15;
		float far = 1000;
		float aspect = 1.0;
		float fov_radian = 0.9;

		/*	*/
		glm::vec4 position = glm::vec4(0);
		glm::vec4 viewDir = glm::vec4(0, 0, 1, 0);
		glm::vec4 position_size = glm::vec4(0);
		glm::uvec4 screen_width_padding = glm::ivec4(1);

		/*	*/
		glm::mat4 view = glm::mat4(1);
		glm::mat4 viewInv = glm::mat4(1);
		glm::mat4 viewRot = glm::mat4(1);
		glm::mat4 viewProj = glm::mat4(1);
		glm::mat4 viewProjInv = glm::mat4(1);
		glm::mat4 proj = glm::mat4(1);
		glm::mat4 inverseProj = glm::mat4(1);
	};

	using FrustumInstance = struct frustum_instance_t {
		frustum_instance_t() = default;
		frustum_instance_t(const Frustum &frustum) {

			for (unsigned int plane_index = 0; plane_index < frustum.getNrPlanes(); plane_index++) {
				planes[plane_index] = glm::vec4(E2GLM(frustum.getPlane(plane_index).getNormal()),
												frustum.getPlane(plane_index).distance());
			}
		}

		glm::vec4 planes[6]{};
	};

} // namespace glsample
