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

	class FVDECLSPEC BlurPostProcessing : public PostProcessing {
		enum Blur { BoxBlur, GuassianBlur, MaxBlur };

	  public:
		BlurPostProcessing();
		~BlurPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

		void renderUI() override;

	  public:
		void render(glsample::FrameBuffer *framebuffer, unsigned int colorTexture, unsigned int read_write_texture);

	  private:
		void updateGuassianKernel();

		int guassian_blur_vertical_compute_program = 0;
		int guassian_blur_horizontal_compute_program = 0;
		int box_blur_compute_program = 0;
		unsigned int texture_sampler = 0;

		Blur blurType = GuassianBlur;

		/*	Settings.	*/
		using BlurSettings = struct blur_settings_t {
			Blur blurType = GuassianBlur;
			int nrIterations = 1;
			float variance = 1;
			int samples = 11;
			float radius = 2;
			float mean = 0;
			static const int maxSamples = 9 + 9 + 1;
			std::array<float, maxSamples> guassian;
		};

		BlurSettings blurSettings;

		int nrIterations = 1;
		float variance = 1;
		int samples = 11;
		float radius = 2;
		float mean = 0;
		static const int maxSamples = 9 + 9 + 1;
		std::array<float, maxSamples> guassian;

		int localWorkGroupSize[3];
	};
} // namespace glsample
