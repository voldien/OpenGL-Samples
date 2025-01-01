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
#include "GLSampleSession.h"
#include "IO/IFileSystem.h"
#include "Math3D/Color.h"
#include "PostProcessing.h"

namespace glsample {

	class FVDECLSPEC PostProcessingManager : public fragcore::Object {
	  public:
		PostProcessingManager() = default;
		~PostProcessingManager() override = default;

		void addPostProcessing(PostProcessing &postProcessing);

		size_t getNrPostProcessing() const noexcept;
		PostProcessing &getPostProcessing(const size_t index) const noexcept;

	  protected:
	};
} // namespace glsample
