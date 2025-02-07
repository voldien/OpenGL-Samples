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

	class FVDECLSPEC ColorGradePostProcessing : public PostProcessing {

	  public:
		ColorGradePostProcessing();
		~ColorGradePostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;
		void renderUI() override;

	  public:
		void render(unsigned int texture);

	  private:
		int hue_color_grade_program = -1;
		int grayscale_color_grade_program = -1;
		int color_balance_program = -1;
		int gamma_program = -1;
		int exposure_program = -1;
		int sepia_program = -1;

		int localWorkGroupSize[3];
	};
} // namespace glsample
