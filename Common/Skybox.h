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
#include "GLDataStructure.h"
#include "Scene/Material.h"
#include "Scene/Node.h"
#include <Exception.hpp>
#include <IO/FileSystem.h>
#include <IO/IOUtil.h>
#include <Scene/CameraController.h>
#include <fmt/format.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace glsample {

	using namespace fragcore;

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC Skybox : public Node {
	  public:
		struct uniform_buffer_block {
			glm::mat4 modelViewProjection;
			glm::vec4 tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			GammaCorrectionSettings correct_settings;
		} FV_ALIGN(4) uniform_stage_buffer;

		Skybox();
		Skybox(const Skybox &) = default;
		Skybox(Skybox &&) = delete;
		Skybox &operator=(const Skybox &) = delete;
		Skybox &operator=(Skybox &&) = delete;
		~Skybox() override;

		virtual void init(unsigned int texture, unsigned int program);

		virtual void render(const Camera &camera) noexcept;
		virtual void render(const glm::mat4 &viewProj) noexcept;
		/**/
		virtual void renderImGUI();

		unsigned int getTexture() const noexcept { return this->skybox_texture_cubemap_panoramic; }
		void setTexture(const unsigned int texture) noexcept { this->skybox_texture_cubemap_panoramic = texture; }

	  public: /*	*/
		static int loadDefaultPanoramicProgram(fragcore::IFileSystem *filesystem);
		static int loadDefaultCubeMapProgram(fragcore::IFileSystem *filesystem);

	  private:
		MeshObject SkyboxCube;
		unsigned int skybox_program;

		int frameIndex = 0;

		/*	Textures.	*/
		unsigned int skybox_texture_cubemap_panoramic;
		unsigned int skybox_sampler;

		Material material;

		/*	Uniform buffer.	*/
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer = 0;
		static constexpr size_t nrUniformBuffer = 3;
		std::array<UBORange, nrUniformBuffer> UniformBuffers;
		size_t uniformAlignSize = sizeof(uniform_buffer_block);

		glm::vec3 rotation;
		bool isEnabled = true;

		bool skybox_settings_visable = true;
	};

} // namespace glsample
