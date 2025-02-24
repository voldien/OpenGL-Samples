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

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC PixelatePostProcessing : public PostProcessing {

	  public:
		PixelatePostProcessing();
		~PixelatePostProcessing() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

		void
		draw(glsample::FrameBuffer *framebuffer,
			 const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets) override;
		void renderUI() override;

	  public:
		using PixelateSettings = struct pixelate_settings_t {
			float pixelSize = 128.f;
		};
		PixelateSettings settings;

	  private:
		int pixelate_graphic_program = -1;
		unsigned int vao = 0;
	};
} // namespace glsample
