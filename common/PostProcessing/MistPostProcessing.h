/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
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
#include "SampleHelper.h"

namespace glsample {

	class FVDECLSPEC MistPostProcessing : public PostProcessing {
	  public:
		MistPostProcessing();
		~MistPostProcessing() override;

		using MistUniformBuffer = struct mist_uniform_buffer_t {
			CameraInstance instance;
			FogSettings fogSettings;
		};

		void initialize(fragcore::IFileSystem *filesystem) override;

		void render(unsigned int skybox, unsigned int frame_texture, unsigned int depth_texture);

	  private:
		int mist_program = -1;
		unsigned int vao = 0;
		unsigned int uniform_buffer = 0;
	};
} // namespace glsample
