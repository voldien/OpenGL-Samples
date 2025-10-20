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
#include "Util/DebugDrawer.h"

namespace glsample {
	/**
	 *
	 */
	class FVDECLSPEC GLDebugDrawManager : public DebugDrawManager {
	  public:
		GLDebugDrawManager(fragcore::IFileSystem *filesystem);
		~GLDebugDrawManager() override = default;

		// TODO: virtual to allow specific render api to handle the command
		//  TODO reduce argument.
		void draw(Camera *camera, FrameBuffer *frame) override;
		//	RenderQueue getSupportedQueue() const override; /*  Render as overlay only. */
		void updateBuffers();

	  protected:
		std::vector<MeshObject> debugGeometrys; /*  Geometry of the debug objects. - multiple sub geometries.   */

		Material material;

		/*	*/
		StageBuffer<DebugData, 3> StageBuffers;
		UBOPool ubo_pool;
		UBOPool storageBufferPool;
	};
} // namespace glsample
