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
#include "GLSampleBase.h"
#include "SampleHelper.h"
#include <cstdint>
#include <initializer_list>

namespace glsample {

	class PostProcessing;
	/*
	 */
	class FVDECLSPEC PostProcessingManager : public fragcore::Object {
	  public:
		PostProcessingManager(GLSampleBase &base);
		~PostProcessingManager() override = default;

		void addPostProcessing(const std::shared_ptr<PostProcessing> &postProcessing);

		size_t getNrPostProcessing() const noexcept;
		PostProcessing &getPostProcessing(const size_t index);

		bool isEnabled(const size_t index) const noexcept;
		void enablePostProcessing(const size_t index, const bool enabled);

		void render(glsample::FrameBuffer *framebuffer,
					const std::initializer_list<std::tuple<const GBuffer, unsigned int>> &render_targets);

		void update(const float deltaTime);

		void populateCommonData() {}
		void swapPostProcessing(int a, int b);

		const UBOPool &getPool() const noexcept { return this->base.getUniformPool(); }
		UBOPool &getPool() noexcept { return this->base.getUniformPool(); }

	  protected:
		std::vector<std::shared_ptr<PostProcessing>> postProcessings;
		std::vector<uint32_t> post_enabled;

		GLSampleBase &base;

	};
} // namespace glsample
