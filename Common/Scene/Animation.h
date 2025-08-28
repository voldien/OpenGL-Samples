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

#include "Core/Object.h"
#include "Importer/ModelImporter.h"

namespace glsample {

	class FVDECLSPEC AnimationPlayer : public fragcore::Object {
	  public:
		AnimationPlayer() = default;
		AnimationPlayer(AnimationObject &animation) {}

		const std::map<std::string, Curve>& getCurves() const noexcept{
			return this->curves;
		}

		float time{};
		unsigned int mode{};
		std::map<std::string, Curve> curves;

	};
} // namespace glsample
