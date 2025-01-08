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
#include "../Common.h"
#include "PostProcessing.h"

namespace glsample {

	class FVDECLSPEC ColorSpaceConverter : public PostProcessing {

	  public:
		ColorSpaceConverter() = default;
		~ColorSpaceConverter() override;

		void initialize(fragcore::IFileSystem *filesystem) override;

	  public:
		void convert(unsigned int texture);

		void setColorSpace(const glsample::ColorSpace srgb) noexcept;
		glsample::ColorSpace getColorSpace() const noexcept;

		GammaCorrectionSettings &getGammeSettings() noexcept { return this->correct_settings; }

	  private:
		ColorSpace colorSpace = ColorSpace::Raw;
		int aes_program = -1;
		int gamma_program = -1;
		int falsecolor_program = -1;
		int kronos_neutral_pbr_program = -1;
		GammaCorrectionSettings correct_settings;

		// int localWorkGroupSize[3];
	};
} // namespace glsample
