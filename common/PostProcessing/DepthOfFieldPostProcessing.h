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

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

	  public:
		void render(unsigned int texture);

	  private:
		unsigned int guassian_blur_compute_program = 0;
		unsigned int indirect_compute_program = 0;

		using DepthOfFieldSettings = struct depth_of_field_settings_t {
			float aperature;
			float Foc;
		};

		/*	*/
		float variance;
		int samples;
		float radius;

		int localWorkGroupSize[3];
	};
} // namespace glsample
