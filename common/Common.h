/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Valdemar Lindberg
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

namespace glsample {

	// TODO: rename
	class FVDECLSPEC Common {
	  public:
		static void loadPlan(MeshObject &planMesh, const float scale, const int segmentX = 1, const int segmentY = 1);
		static void loadCube(MeshObject &planMesh);
		static void loadSphere(MeshObject &sphereMesh, const float radius = 1, const int slices = 8,
							   const int segements = 8);
	};

	extern void refreshWholeRoundRobinBuffer(unsigned int bufferType, unsigned int buffer, const unsigned int robin,
											 const void *data, const size_t alignSize, const size_t bufferSize,
											 const size_t offset = 0);

} // namespace glsample