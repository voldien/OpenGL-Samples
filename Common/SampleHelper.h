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
#include "DataStructure/MemoryAddress.h"
#include "RenderDesc.h"
#include <IO/IOUtil.h>
#include <Math3D/AABB.h>
#include <Math3D/Color.h>
#include <Math3D/Math3D.h>
#include <Math3D/OBB.h>
#include <array>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>

namespace glsample {

	using IOUtil = fragcore::IOUtil;

	template <typename T, unsigned int n> struct StageBuffer {
	  public:
		T getBase() const noexcept { return this->buffers[0]; }
		T &getBuffer(const unsigned int index) noexcept { return this->buffers[index]; }
		std::array<T, n> buffers;
	};

	enum class GBuffer : unsigned int {
		Albedo = 0,				 /*	*/
		Color = 0,				 /*	Color, Alpha	*/
		WorldSpace = 1,			 /*	WorldSpace (XYZ), */
		TextureCoordinate = 2,	 /*	TexCoord0, TexCoord1	*/
		Normal = 3,				 /*	Normal XYZ, */
		Specular = 4,			 /* SpecularColor,	Roughness.	*/
		Emission = 5,			 /*	Emission Color (RGB), */
		Depth = 6,				 /*	*/
		Velocity = 7,			 /*	Velocity (XYZ), */
		RoughnessMetalAO = 8,	 /*	*/
		Roughness = 8,			 /*	*/
		AO = 9,					 /*	*/
		Displacement = 10,		 /*	*/
		Metallic = 11,			 /*	*/
		SubSurface = 12,		 /*	*/
		LightPass = 13,			 /*	*/
		IntermediateTarget = 14, /*	*/
		IntermediateTarget2 = 15 /*	*/
	};

	using UBOObject = struct uniform_buffer_object_t {
		unsigned int buffer;	/*	*/
		size_t size;			/*	*/
		size_t totalSize;		/*	*/
		unsigned int alignment; /*	*/
	};

	using UBORange = struct uniform_buffer_range_t {
		UBOObject *referenceBuffer; /*	*/
		size_t offset;				/*	*/
		size_t size;				/*	*/
	};

	using UBOPool = struct uniform_buffer_pool_object_t {
		UBOObject buffer{};
		fragcore::MemoryAddress addresser;
	};

	using TextureSampler = struct texture_sampling_t {
		unsigned int sampler{};
		fragcore::TextureWrappingMode wrapping = fragcore::TextureWrappingMode::Repeat;
		fragcore::TextureFilterMode filtering = fragcore::TextureFilterMode::Linear;
		fragcore::TextureUVMappingMode uv_mapping = fragcore::TextureUVMappingMode::UV;
		unsigned int LODBias{};
		unsigned int minLOD{};
		unsigned int maxLOD{};
		unsigned int ansiotropy{};
	};

	using Texture = struct texture_t {
		unsigned int texture_type{};
		unsigned int texture{};
		unsigned int width{};
		unsigned int height{};
		unsigned int depth = 1;
		unsigned int numSamples = 1;
	};

	using FrameBuffer = struct framebuffer_t {
		unsigned int framebuffer = 0;
		std::array<unsigned int, 32> attachments{}; /*	last */
		std::array<glm::ivec3, 32> attachmentSize{};
		std::array<unsigned int, 32> draw_attachments{}; /*	Store the draw attachment for */
		unsigned int nrAttachments = 0;
		unsigned int depthIndex = 31; /*	Last attachment reserved for the depth/stencil.	*/
	};

	/*	*/
	using TextureDesc = fragcore::TextureDesc;
	using SamplerDesc = fragcore::SamplerDesc;

} // namespace glsample
