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

namespace glsample {

	class FVDECLSPEC SSReflectionPostProcessing : public PostProcessing {

	  public:
		SSReflectionPostProcessing();
		~SSReflectionPostProcessing() override;

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
			uint max_steps = 16;			// Max ray steps, affects quality and performance.
			float ray_max_distance = 0.05f; // Max shadow length, longer shadows are less accurate.
			float thickness = 0.02f;		// Depth testing thickness.
			float step_length = ray_max_distance / float(max_steps);
			glm::vec2 taa_jitter_offset = glm::vec2(0);
			glm::vec3 light_direction = glm::vec3(-1, -1, 0);
		};
		ScreenSpaceSettings SSSSettings;

		unsigned int world_position_sampler = 0;
		// int localWorkGroupSize[3];
		int vao;
	};
} // namespace glsample
