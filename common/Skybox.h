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
#include "Core/UIDObject.h"
#include "FragDef.h"
#include "GLSampleSession.h"
#include <Exception.hpp>
#include <IO/FileSystem.h>
#include <IO/IOUtil.h>
#include <Util/CameraController.h>
#include <fmt/format.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace glsample {
	using namespace fragcore;

	class FVDECLSPEC Skybox : public UIDObject {
	  public:
		struct uniform_buffer_block {
			glm::mat4 proj;
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			GammaCorrectionSettings correct_settings;
		} FV_ALIGN(4) uniform_stage_buffer;

		Skybox();
		Skybox(const Skybox &) = default;
		Skybox(Skybox &&) = delete;
		Skybox &operator=(const Skybox &) = delete;
		Skybox &operator=(Skybox &&) = delete;
		virtual ~Skybox();

		virtual void Init(unsigned int texture, unsigned int program);

		virtual void Render(const CameraController &camera);
		virtual void Render(const glm::mat4 &viewProj);
		/**/
		virtual void RenderImGUI();

		unsigned int getTexture() const noexcept { return this->skybox_texture_panoramic; }

	  public:
		static int loadDefaultProgram(fragcore::IFileSystem *filesystem);

	  private:
		MeshObject SkyboxCube;
		unsigned int skybox_program;

		int frameIndex = 0;

		unsigned int skybox_texture_panoramic;
		unsigned int skybox_sampler;
		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer;
		const size_t nrUniformBuffer = 3;
		size_t uniformAlignSize = sizeof(uniform_buffer_block);
		glm::vec3 rotation;
		bool isEnabled = true;

		bool skybox_settings_visable = true;
	};

} // namespace glsample