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
#include "GLSampleSession.h"
#include "IO/IFileSystem.h"
#include "Math3D/Color.h"
#include "SampleHelper.h"

namespace glsample {

	class FVDECLSPEC PostProcessing : public fragcore::Object {
	  public:
		PostProcessing();
		~PostProcessing() override = default;

		virtual void initialize(fragcore::IFileSystem *filesystem) = 0;

		virtual void bind() {};

		virtual void draw(glsample::FrameBuffer *framebuffer,
						  const std::initializer_list<std::tuple<const GBuffer, const unsigned int &>> &render_targets);

		virtual void renderUI() {};

		float getIntensity() const noexcept;
		void setItensity(const float intensity);

		bool isBufferRequired(const GBuffer required_data_buffer) const noexcept;

		/*	Common data references.	*/

	  protected:
		void addRequireBuffer(const GBuffer required_data_buffer) noexcept;
		void removeRequireBuffer(const GBuffer required_data_buffer) noexcept;

		const unsigned int &getMappedBuffer(const GBuffer buffer_target) const noexcept;

		int createVAO();

		int createOverlayGraphicProgram(fragcore::IFileSystem *filesystem);

	  protected:
		std::vector<GBuffer> required_buffer;
		std::map<GBuffer, const unsigned int *> mapped_buffer;
		bool computeShaderSupported = true;
		float intensity = 1;
	};
} // namespace glsample
