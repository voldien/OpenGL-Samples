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
#include <array>

namespace glsample {

	/*	The higher the later the will be rendered in the rendering queue.	*/
	enum RenderQueue : unsigned int {
		Background = 500,	 /*  */
		Geometry = 1000,	 /*  */
		AlphaTest = 1500,	 /*  */
		GeometryLast = 1600, /*  */
		Transparent = 2000,	 /*  */
		Overlay = 3000,		 /*  */
	};

	static const std::array<RenderQueue, 6> &getQueueTypesOrdered() noexcept {
		static const std::array<RenderQueue, 6> order = {RenderQueue::Background,  RenderQueue::Geometry,
														 RenderQueue::AlphaTest,   RenderQueue::GeometryLast,
														 RenderQueue::Transparent, RenderQueue::Overlay};

		return order;
	}

	static const std::array<const char *, 6> &getRenderQueueSymbols() noexcept {
		static const std::array<const char *, 6> symbols = {"Background",	"Geometry",	   "AlphaTest",
															"Geometrylast", "Transparent", "Overlay"};
		return symbols;
	}
} // namespace glsample