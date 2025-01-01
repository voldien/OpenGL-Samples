/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
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
#include <Eigen/Eigen>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

namespace glsample {

	enum class GBuffer : unsigned int {
		WorldSpace = 1,		   /*	*/
		TextureCoordinate = 2, /*	*/
		Albedo = 0,			   /*	*/
		Normal = 3,			   /*	*/
		Specular = 4,		   /*	Roughness*/
		Emission = 5,		   /*	*/
	};

	// TODO: relocate
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
		glm::vec4 fogColor = glm::vec4(0.3, 0.3, 0.45, 1);

		float cameraNear = 0.15f;
		float cameraFar = 1000.0f;
		float fogStart = 100;
		float fogEnd = 1000;

		float fogDensity = 0.1f;
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

	using DirectionalLight = struct directional_light_t {
		glm::vec4 lightDirection = glm::vec4(1.0f / sqrt(2.0f), -1.0f / sqrt(2.0f), 0, 0.0f);
		glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	};

	using PointLightInstance = struct point_light_instance_t {
		glm::vec3 position;
		float range;
		glm::vec4 color;
		/*	*/
		float intensity;
		float constant_attenuation;
		float linear_attenuation;
		float quadratic_attenuation;
	};

	using CameraInstance = struct camera_instance_t {
		float near;
		float far;
		float aspect;
		float fov;
		glm::vec4 position;
		glm::vec4 viewDir;
		glm::vec4 position_size;
	};

	/*	Default framebuffer.	*/
	using FrameBuffer = struct framebuffer_t {
		unsigned int framebuffer;
		unsigned int attachement0;
		unsigned int intermediate;
		unsigned int depthbuffer;
	};

	template <typename T, int m, int n>
	inline glm::mat<m, n, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, n> &em) noexcept {
		glm::mat<m, n, float, glm::precision::highp> mat;
		for (int i = 0; i < m; ++i) {
			for (int j = 0; j < n; ++j) {
				mat[j][i] = em(i, j);
			}
		}
		return mat;
	}

	// template <typename A, class T...> struct align_uniform {};

	template <typename T, int m>
	inline glm::vec<m, float, glm::precision::highp> E2GLM(const Eigen::Matrix<T, m, 1> &em) noexcept {
		glm::vec<m, float, glm::precision::highp> v;
		for (int i = 0; i < m; ++i) {
			v[i] = em(i);
		}
		return v;
	}

	template <typename T, int m> inline Eigen::Matrix<T, m, 1> GLM2E(const glm::vec<m, T> &em) noexcept {
		Eigen::Matrix<T, m, 1> v;
		for (int i = 0; i < m; ++i) {
			v(i) = em[i];
		}
		return v;
	}

	template <typename T, int m, int n> inline Eigen::Matrix<T, m, n> GLM2E(const glm::mat<m, n, T> &em) noexcept {
		Eigen::Matrix<T, m, n> mat;
		for (int i = 0; i < m; ++i) {
			for (int j = 0; j < n; ++j) {
				mat(j, i) = em[i][j];
			}
		}
		return mat;
	}
} // namespace glsample