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

	class FVDECLSPEC BloomPostProcessing : public PostProcessing {

	  public:
		BloomPostProcessing();
		~BloomPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;

	  public:
		void convert(FrameBuffer *framebuffer, unsigned int texture);

	  private:
		int bloom_blur_graphic_program = -1;
		int overlay_program = -1;
		int downsample_compute_program = -1;
		int upsample_compute_program = -1;

		unsigned int texture_sampler = 0;

		/*	*/
		float variance;
		int samples;
		float radius;
		float mean = 0;
		int localWorkGroupSize[3];
		float threadshold = 1;
		int vao = 0;
	};
} // namespace glsample
