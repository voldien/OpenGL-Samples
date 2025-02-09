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
#include <glm/fwd.hpp>

namespace glsample {

	class FVDECLSPEC SSSPostProcessing : public PostProcessing {

	  public:
		SSSPostProcessing();
		~SSSPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

		void renderUI() override;

	  private:
		int screen_space_shadow_frag_program = -1;
		int overlay_program = -1;

		using ScreenSpaceSettings = struct screen_space_settings_t {
			float blend = 1;
			uint g_sss_max_steps = 16;			  // Max ray steps, affects quality and performance.
			float g_sss_ray_max_distance = 0.05f; // Max shadow length, longer shadows are less accurate.
			float g_sss_thickness = 0.02f;		  // Depth testing thickness.
			float g_sss_step_length = g_sss_ray_max_distance / float(g_sss_max_steps);
			glm::vec2 g_taa_jitter_offset = glm::vec2(0);
			glm::vec3 light_direction = glm::vec3(-1, -1, 0);
		};
		ScreenSpaceSettings SSSSettings;

		unsigned int world_position_sampler = 0;
		// int localWorkGroupSize[3];
		int vao;
	};
} // namespace glsample
