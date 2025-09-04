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
#include "Math3D/Color.h"
#include "RenderDesc.h"
#include "SampleHelper.h"
#include <initializer_list>

namespace glsample {

	using DrawArraysIndirectCommand = fragcore::IndirectDrawArray;
	using DrawElementsIndirectCommand = fragcore::IndirectDrawElement;
	using DrawDispatchIndirectCommand = fragcore::IndirectDispatch;

	using MeshObject = struct geometry_object_t {
		/*	*/
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ibo = 0;

		size_t nrIndicesElements = 0;
		size_t nrVertices = 0;

		size_t vertex_offset = 0;
		size_t indices_offset = 0;

		unsigned int stride = 0;
		int primitiveType = 0;

		/*	*/
		fragcore::Bound bound{};
	};

	using TextureObject = struct texture_object_t {
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int depth = 0;
		unsigned int texture = 0;
	};

	enum class ColorSpace : unsigned int {
		RawLinear = 0,	   /*	Linear.	*/
		SRGB,			   /*	SRGB encoded.	*/
		ACES,			   /*	*/
		Filmic,			   /*	*/
		KhronosPBRNeutral, /*	*/
		FalseColor,		   /*	*/
		MaxColorSpaces
	};

	class FVDECLSPEC CommonUtil {
	  public:
		static void loadPlan(MeshObject &planMesh, const float scale, const int segmentX = 1, const int segmentY = 1);
		static void loadCube(MeshObject &cubeMesh, const float scale, const int segmentX = 1, const int segmentY = 1);
		static void loadSphere(MeshObject &sphereMesh, const float radius = 1, const int slices = 8,
							   const int segements = 8);

		static void mergeMeshBuffers(const std::vector<MeshObject> &sphereMesh, std::vector<MeshObject> &mergeMeshes);

		static int createColorTexture(unsigned int width, unsigned int height, const fragcore::Color &color);
		static int createColorTexture16F(unsigned int width, unsigned int height, const fragcore::Color &color);

		static void createFrameBuffer(FrameBuffer *framebuffer, unsigned int nrAttachments);
		static void updateFrameBuffer(FrameBuffer *framebuffer,
									  const std::initializer_list<fragcore::TextureDesc> &desc,
									  const fragcore::TextureDesc &depthstencil);
	};

	extern void refreshWholeRoundRobinBuffer(unsigned int bufferType, unsigned int buffer, const unsigned int robin,
											 const void *data, const size_t alignSize, const size_t bufferSize,
											 const size_t offset = 0);
	template <typename T>
	void refreshWholeRoundRobinBuffer(unsigned int bufferType, unsigned int buffer, const unsigned int robin,
									  const T *data, const size_t alignSize, const size_t bufferSize = sizeof(T),
									  const size_t offset = 0) {
		refreshWholeRoundRobinBuffer(bufferType, buffer, robin, (const void *)data, alignSize, bufferSize, offset);
	}

} // namespace glsample
