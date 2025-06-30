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
#include "Buffer.h"
#include "PostProcessing.h"

namespace glsample {

	/**
	 *
	 */
	class FVDECLSPEC AtmosphericScattering : public PostProcessing {

	  public:
		AtmosphericScattering();
		~AtmosphericScattering() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void draw(glsample::FrameBuffer *framebuffer,
				  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) override;

		void renderUI() override;

		bool isSupported() const noexcept override { return computeShaderSupported; }

	  public:
		void render(FrameBuffer *framebuffer, unsigned int color_texture);

	  private:
		int atmospheric_scattering_graphic_program = -1;
		int overlay_program = -1;
		UBORange bufferRange;

		unsigned int texture_sampler = 0;

		using PlanetSettings = struct planet_settings_t {
			glm::vec4 center_radius;
			glm::vec4 atmos_radius;
		};

		using AtmosphericScatteringSettings = struct atmospheric_scattering_t {
			int numSamples = 64;
			glm::vec3 sunDirection; // The sun direction (normalized)

			/*	*/
			float earthRadius;		// In the paper this is usually Rg or Re (radius ground, eart)
			float atmosphereRadius; // In the paper this is usually R or Ra (radius atmosphere)

			float Hr; // Thickness of the atmosphere if density was uniform (Hr)
			float Hm; // Same as above but for Mie scattering (Hm)

			int nrAtmoSpheres;

			PlanetSettings planets[32];

			glm::vec4 betaR;
			glm::vec4 betaM;
		};

		AtmosphericScatteringSettings settings;

		int localWorkGroupSize[3];

		int vao = 0;
	};
} // namespace glsample
