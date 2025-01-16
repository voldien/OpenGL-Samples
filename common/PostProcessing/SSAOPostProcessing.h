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

	class FVDECLSPEC SSAOPostProcessing : public PostProcessing {

	  public:
		SSAOPostProcessing();
		~SSAOPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

	  public:
		void convert(unsigned int texture);

	  private:
		int ssao_depth_only_program = -1;
		int ssao_depth_world_program = -1;
		int overlay_program = -1;
		int downsample_compute_program = -1;

		int uniform_ssao_buffer_binding = 0;

		/*	*/
		float variance;
		int samples;
		float radius;
		static const int maxKernels = 64;

		struct UniformSSAOBufferBlock {
			glm::mat4 proj{};

			/*	*/
			int samples = 64;
			float radius = 2.5f;
			float intensity = 0.8f;
			float bias = 0.025;

			glm::vec4 kernel[maxKernels]{};

			glm::vec4 color{};
			glm::vec2 screen{};
			CameraInstance camera;

		} uniformStageBlockSSAO;

		size_t uniformSSAOBufferAlignSize = sizeof(UniformSSAOBufferBlock);
		unsigned int uniform_ssao_buffer = 0;
		/*	Random direction texture.	*/
		unsigned int random_texture = 0;
		unsigned int white_texture = 0;
		unsigned int vao = 0;

		// int localWorkGroupSize[3];
	};
} // namespace glsample
