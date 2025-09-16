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

#include "Common.h"
#include "FragDef.h"
#include "Frustum.h"
#include "GLDataStructure.h"
#include "RenderDesc.h"
#include "SampleHelper.h"

namespace glsample {

	class FVDECLSPEC Light : public Frustum {
	  public:
		enum class LightType {
			Directional, /*	*/
			Point,		 /*	*/
			Spot,		 /*	*/
			Area,		 /*	*/
			MaxLightType /*	*/
		};

		glm::vec3 getDirectionalLight() const noexcept;

		float getShadowStrength() const noexcept;
		void setShadowStrength(float strength);

		virtual void setShadowDistance(float distance);
		float getShadowDistance() const noexcept;

		const glm::mat4 &getProjectionMatrix() const noexcept;

		LightType getLightType() const noexcept;

		FrameBuffer *getFrameBuffer() const noexcept;

		/*	*/
		virtual void setSize(const glm::ivec3 &size);
		glm::ivec3 getSize() const noexcept;

		/*	*/
		glm::vec4 getColor() const noexcept;
		void setColor(const glm::vec4 &newColor);

		bool hasShadow() const noexcept { return this->getShadowStrength() > 0 && this->getFrameBuffer(); }

	  public:
		LightType lightType = LightType::Directional;
		glm::vec4 color = glm::vec4(1);

		LightDirectionalShadowData shadowData{};

		float shadow = 1;
		float bias = 0.0002f;
		float shadowDistance = 50;
		float pcf_radius = 1;
		FrameBuffer *shadowFrameBuffer = nullptr;
	};

	class FVDECLSPEC DirectionalLight : public Light {
	  public:
		DirectionalLight();

		void setSize(const glm::ivec3 &size) override;
		void setShadowDistance(float distance) override;
		void calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
							   const glm::vec3 &right) override;
	};

	class FVDECLSPEC PointLight : public Light {
	  public:
		PointLight() {
			this->setName("PointLight");
			this->lightType = LightType::Point;
			this->setShadowDistance(50.0f);
		}

		void setSize(const glm::ivec3 &size) override {

			if (size[0] > 0 && size[1] > 0) {

				GraphicFormat internal_depth_format = GraphicFormat::Depth_16Bit;
				fragcore::TextureDesc desc;
				desc.target = fragcore::TextureDesc::TextureTarget::CubeMap;
				desc.width = size[0];
				desc.height = size[1];
				desc.depth = 1;
				desc.graphicFormat = internal_depth_format;
				desc.nrSamples = 0;

				if (!this->getFrameBuffer()) {
					shadowFrameBuffer = new FrameBuffer();
					CommonUtil::createFrameBuffer(shadowFrameBuffer, 0);
				}

				if (this->getFrameBuffer()) {
					CommonUtil::updateFrameBuffer(getFrameBuffer(), {}, &desc);
				}
			}
		}

		void setShadowDistance(float distance) override {
			Light::setShadowDistance(distance);

			const float near_plane = -getShadowDistance();
			const float far_plane = getShadowDistance();

			const glm::mat4 lightProjection =
				glm::ortho(-getShadowDistance(), getShadowDistance(), -getShadowDistance(), getShadowDistance(),
						   near_plane, far_plane);

			const glm::vec3 light_direction = this->getDirectionalLight();

			const glm::mat4 lightView =
				glm::lookAt(this->getPosition(), this->getPosition() + this->getDirectionalLight(), this->up());
			const glm::mat4 lightSpaceMatrix = lightProjection * lightView;

			shadowData.lightSpaceMatrix = lightSpaceMatrix;

			this->calcFrustumPlanes(this->getPosition(), this->getDirectionalLight(), this->up(), this->right());
		}

		void calcFrustumPlanes(const glm::vec3 &position, const glm::vec3 &look_forward, const glm::vec3 &up,
							   const glm::vec3 &right) override {
			const float distance = this->getShadowDistance();

			// this->planes[NEAR_PLANE] = {position - distance * look_forward, look_forward};
			// this->planes[FAR_PLANE] = {position + distance * look_forward, -look_forward};

			// this->planes[RIGHT_PLANE] = {position - distance * right, right};
			// this->planes[LEFT_PLANE] = {position + distance * right, -right};

			// this->planes[TOP_PLANE] = {position - distance * up, up};
			// this->planes[BOTTOM_PLANE] = {position + distance * up, -up};
		}

		float getRange() const noexcept { return this->range; }

		float intensity = 1;
		float range = 5;
		float constant_attenuation = 1;
		float linear_attenuation = 0.1f;
		float quadratic_attenuation = 0.025f;
	};
} // namespace glsample
