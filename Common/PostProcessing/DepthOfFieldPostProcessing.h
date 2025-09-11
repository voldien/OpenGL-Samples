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

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC DepthOfFieldProcessing : public PostProcessing {
	  public:
		DepthOfFieldProcessing();
		~DepthOfFieldProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void draw(glsample::FrameBuffer *framebuffer,
				  const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets) override;
		void renderUI() override;

	  public:
		void render(unsigned int texture);

	  private:
		void updateGuassianKernel();

		unsigned int circle_of_confusion_compute_program = 0;

		unsigned int guassian_blur_variable_horizontal_compute_program = 0;
		unsigned int guassian_blur_variable_vertical_compute_program = 0;

		unsigned int guassian_blur_vertical_fixed_compute_program = 0;
		unsigned int guassian_blur_horizontal_fixed_compute_program = 0;

		unsigned int indirect_guassian_dispatch_compute_program = 0;

		unsigned int bokeh_vertex_program = 0;
		unsigned int bokeh_fragment_program = 0;

		unsigned int indirect_bokeh_compute_program = 0;

		unsigned int texture_sampler = 0;
		unsigned int vao = 0;

		/*	Settings.	*/
		using DepthOfFieldSettings = struct depth_of_field_settings_t {
			float aperature;
			float Foc;
			int nrIterations = 1;
			float radius = 2;
			float variance = 1;
			float mean = 0;
			int samples = 11;
			static const int maxSamples = 9 + 9 + 1;
			std::array<float, maxSamples> guassian;
		};

		DepthOfFieldSettings blurSettings;

		int depthOfFieldType;

		std::array<std::array<int, 3>, 5> localWorkGroupSize;
	};
} // namespace glsample
