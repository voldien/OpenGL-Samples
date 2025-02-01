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
#include "Util/Camera.h"
#include "Util/CameraController.h"
#include <Eigen/Eigen>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

namespace glsample {

	enum class GBuffer : unsigned int {
		Albedo = 0,			   /*	*/
		Color = 0,			   /*	*/
		WorldSpace = 1,		   /*	*/
		TextureCoordinate = 2, /*	*/
		Normal = 3,			   /*	*/
		Specular = 4,		   /*	Roughness*/
		Emission = 5,		   /*	*/
		Depth = 6,			   /*	*/
		Velocity = 7,		   /*	*/
		Roughness = 8,		   /*	*/
		AO,
		Displacement,
		Metallic,			/*	*/
		SubSurface,			/*	*/
		LightPass,			/*	*/
		IntermediateTarget, /*	*/
		IntermediateTarget2 /*	*/
	};

	enum class FogType : unsigned int {
		None,	/*	*/
		Linear, /*	*/
		Exp,	/*	*/
		Exp2,	/*	*/
		Height	/*	*/
	};

	using UnfiformSubBuffer = struct uniform_sub_buffer_t {};

	using GammaCorrectionSettings = struct alignas(16) gamme_correct_settings_t {
		float exposure = 1.0f;
		float gamma = 2.2f;
	};

	using FogSettings = struct alignas(16) fog_settings_t {
		glm::vec4 fogColor = glm::vec4(0.3, 0.3, 0.45, 1);
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

	using MaterialInstance = struct alignas(16) material_instance_t {
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

	using BlinnPhongMaterialData = struct alignas(16) blinn_phong_material_data_t {};

	using DirectionalLight = struct directional_light_t {
		glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
		glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	};

	using PointLightInstance = struct alignas(16) point_light_instance_t {
		glm::vec3 position;
		float range;
		glm::vec4 color;
		/*	*/
		float intensity;
		float constant_attenuation;
		float linear_attenuation;
		float quadratic_attenuation;
	};

	using CameraInstance = struct alignas(16) camera_instance_t {
		/*	*/
		camera_instance_t &operator=(Camera &camera) {
			this->near = camera.getNear();
			this->far = camera.getFar();
			this->proj = camera.getProjectionMatrix();
			this->inverseProj = glm::inverse(camera.getProjectionMatrix());

			return *this;
		}
		/*	*/
		camera_instance_t &operator=(CameraController &camera) {
			//*this = camera.as<Camera<float>>();
			// TODO: reuse function above
			this->near = camera.getNear();
			this->far = camera.getFar();
			this->proj = camera.getProjectionMatrix();
			this->inverseProj = glm::inverse(camera.getProjectionMatrix());

			this->near = camera.getNear();
			this->far = camera.getFar();
			this->position = glm::vec4(camera.getPosition(), 0);
			this->viewDir = glm::vec4(camera.getLookDirection(), 0);
			this->view = camera.getViewMatrix();
			this->viewProj = this->proj * this->view;
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
		glm::mat4 viewProj = glm::mat4(1);
		glm::mat4 proj = glm::mat4(1);
		glm::mat4 inverseProj = glm::mat4(1);
	};

	using FrustumInstance = struct alignas(16) frustum_instance_t {
		glm::vec4 planes[6];
	};

	using UBOObject = struct uniform_buffer_object_t {
		unsigned int buffer;
		unsigned int size;
		unsigned int totalSize;
		unsigned int alignment;
	};

	using FrameBuffer = struct framebuffer_t {
		unsigned int framebuffer = 0;
		std::array<unsigned int, 16> attachments;
		unsigned int nrAttachments = 0;
		unsigned int depthbuffer = 0;
	};

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
		glm::vec<m, float, glm::precision::highp> v;
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
} // namespace glsample