/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */
#pragma once
#include "DataStructure/MemoryAddress.h"
#include "Math3D/LinAlg.h"
#include "Util/Camera.h"
#include "Util/CameraController.h"
#include <Eigen/Eigen>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

namespace glsample {

	template <typename T, int m, int n>
	inline glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n> &em) noexcept {
		glm::mat<m, n, float, glm::precision::highp> mat;
		for (unsigned int i = 0; i < m; ++i) {
			for (unsigned int j = 0; j < n; ++j) {
				mat[j][i] = em(i, j);
			}
		}
		return mat;
	}

	// template <typename A, class T...> struct align_uniform {};

	template <typename T, int m>
	inline glm::vec<m, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, 1> &em) noexcept {
		glm::vec<m, float, glm::precision::highp> v{};
		for (unsigned int i = 0; i < m; ++i) {
			v[i] = em(i);
		}
		return v;
	}

	template <typename T, int m> inline Eigen::Matrix<T, m, 1> GLM2E(const glm::vec<m, T> &em) noexcept {
		Eigen::Matrix<T, m, 1> v;
		for (unsigned int i = 0; i < m; ++i) {
			v(i) = em[i];
		}
		return v;
	}

	template <typename T, int m, int n> inline Eigen::Matrix<T, m, n> GLM2E(const glm::mat<m, n, T> &em) noexcept {
		Eigen::Matrix<T, m, n> mat;
		for (unsigned int i = 0; i < m; ++i) {
			for (unsigned int j = 0; j < n; ++j) {
				mat(j, i) = em[i][j];
			}
		}
		return mat;
	}

	enum class GBuffer : unsigned int {
		Albedo = 0,				 /*	*/
		Color = 0,				 /*	Color, Alpha	*/
		WorldSpace = 1,			 /*	*/
		TextureCoordinate = 2,	 /*	TexCoord0, TexCoord1	*/
		Normal = 3,				 /*	*/
		Specular = 4,			 /* SpecularColor,	Roughness*/
		Emission = 5,			 /*	*/
		Depth = 6,				 /*	*/
		Velocity = 7,			 /*	*/
		Roughness = 8,			 /*	*/
		AO = 9,					 /*	*/
		Displacement = 10,		 /*	*/
		Metallic = 11,			 /*	*/
		SubSurface = 12,		 /*	*/
		LightPass = 13,			 /*	*/
		IntermediateTarget = 14, /*	*/
		IntermediateTarget2 = 15 /*	*/
	};

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

	using MaterialInstance = struct material_instance_t {
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

	using LightShadow = struct light_shadow_t {
		glm::mat4 lightSpaceMatrix;
		glm::vec4 shadow;
	};

	using DirectionalLight = struct directional_light_t {
		LightShadow lightShadow;
		glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
		glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	};

	using PointLightInstance = struct point_light_instance_t {
		LightShadow lightShadow;
		glm::vec3 position = glm::vec3(0);
		float range = 5;
		glm::vec4 color = glm::vec4(1);
		/*	*/
		float intensity = 1;
		float constant_attenuation = 1;
		float linear_attenuation = 0.1f;
		float quadratic_attenuation = 0.025f;
	};

	using CameraInstanceData = struct camera_instance_data_t {
		/*	*/
		camera_instance_data_t &operator=(const Camera &camera) {
			this->near = camera.getNear();
			this->far = camera.getFar();
			this->proj = camera.getProjectionMatrix();
			this->inverseProj = glm::inverse(camera.getProjectionMatrix());

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

		float near = 0.15;
		float far = 1000;
		float aspect = 1.0;
		float fov_radian = 0.9;

		glm::vec4 position = glm::vec4(0);
		glm::vec4 viewDir = glm::vec4(0, 0, 1, 0);
		glm::vec4 position_size = glm::vec4(0);
		glm::uvec4 screen_width_padding = glm::ivec4(1);

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

			for (unsigned int i = 0; i < 6; i++) {
				planes[i] = glm::vec4(E2GLM(frustum.getPlane(i).getNormal()), frustum.getPlane(i).distance());
			}
		}

		glm::vec4 planes[6]{};
	};

	using UBOObject = struct uniform_buffer_object_t {
		unsigned int buffer;	/*	*/
		size_t size;			/*	*/
		size_t totalSize;		/*	*/
		unsigned int alignment; /*	*/
	};

	using UBOPool = struct uniform_buffer_pool_object_t {
		UBOObject buffer{};
		MemoryAddress addresser;
	};

	using UBORange = struct uniform_buffer_range_t {
		UBOObject *referenceBuffer; /*	*/
		size_t offset;		  /*	*/
		size_t size;		  /*	*/
	};

	using FrameBuffer = struct framebuffer_t {
		unsigned int framebuffer = 0;
		std::array<unsigned int, 32> attachments{}; /*	last */
		std::array<glm::ivec3, 32> attachmentSize{};
		std::array<unsigned int, 32> draw_attachments{}; /*	Store the draw attachment for */
		unsigned int nrAttachments = 0;
		unsigned int depthIndex = 31;	/*	Last attachment reserved for the depth/stencil.	*/
	};

} // namespace glsample