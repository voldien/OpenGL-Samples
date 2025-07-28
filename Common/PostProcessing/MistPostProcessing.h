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
#include "PostProcessing.h"
#include "SampleHelper.h"

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC MistPostProcessing : public PostProcessing {
	  public:
		MistPostProcessing();
		~MistPostProcessing() override;

		using MistUniformBuffer = struct mist_uniform_buffer_t {
			glm::mat4 proj;
			glm::mat4 viewRotation;
			CameraInstanceData instance;
			FogSettings fogSettings;
			float density; // Scattering density
			float betaR;   // Rayleigh scattering coefficient
			float betaM;   // Mie scattering coefficient
			float g;	   // Mie phase function parameter
			DirectionalLight sunLight;
		};

		void initialize(fragcore::IFileSystem *filesystem) override;

		void draw(glsample::FrameBuffer *framebuffer,
				  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) override;

		void render(unsigned int skybox, unsigned int frame_texture, unsigned int depth_texture);

		void renderUI() override;

		MistUniformBuffer mistsettings;

		enum class MistType : unsigned int {
			SimpleFog = 0,
			IrradianceFog = 1,
			RayLeighMie = 2,
		};

	  private:
		int mist_fog_program = -1;
		int simple_fog_program = 0;
		int rayleighmie_program = 0;
		unsigned int vao = 0;

		unsigned int texture_sampler = 0;

		// Rayleigh and Mie

		MistType mistType = MistType::SimpleFog;

		unsigned int uniform_buffer = 0;

		size_t uniformAlignSize = sizeof(MistUniformBuffer);

		/*	*/
		unsigned int uniform_buffer_binding = 1;
		std::array<UBORange, 2> buffers;
		unsigned int buffer_use_index = 0;
	};
} // namespace glsample
