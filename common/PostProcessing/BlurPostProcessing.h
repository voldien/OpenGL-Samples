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

	  public:
		BlurPostProcessing();
		~BlurPostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void draw(const std::initializer_list<std::tuple<GBuffer, unsigned int>> &render_targets) override;

	  public:
		void convert(unsigned int texture);

	  private:
		int guassian_blur_compute_program = -1;

		/*	*/
		float variance;
		int samples;
		float radius;
		float mean = 0;

		int localWorkGroupSize[3];
	};
} // namespace glsample
