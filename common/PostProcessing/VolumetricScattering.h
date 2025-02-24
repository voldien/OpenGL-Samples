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
#include <glm/fwd.hpp>

namespace glsample {

	class FVDECLSPEC VolumetricScatteringPostProcessing : public PostProcessing {

	  public:
		VolumetricScatteringPostProcessing();
		~VolumetricScatteringPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

		void renderUI() override;

	  private:
		int volumetric_scattering_legacy_program = -1;
		int downsample_compute_program = -1;

		unsigned int texture_sampler = 0;

		using VolumetricScatteringSettings = struct volumetric_scattering_settings_t {
			int numSamples = 64;
			float _Density = 0.345;
			float _Decay = 0.88;
			float _Weight = 2.8;
			float _Exposure = 4.2;
			glm::vec2 lightPosition = glm::vec2(0.5f, 0.8);
			glm::vec4 color = glm::vec4(1, 1, 1, 1);
		};

		VolumetricScatteringSettings volumetricScatteringSettings;

		/*	Random direction texture.	*/
		unsigned int vao = 0;
	};
} // namespace glsample
