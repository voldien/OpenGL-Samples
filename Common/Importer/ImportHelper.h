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
#include "Common.h"
#include "ModelImporter.h"

namespace glsample {

	enum AttributeMapping : unsigned int {
		Vertex = 0,
		UV = 1,
		ANormal = 2,
		ATangent = 3,
		ABoneIndex = 4,
		ABoneWeight = 5,
		AVertexColor = 6,
	};

	class FVDECLSPEC ImportHelper {
	  public:
		/**
		 * @brief
		 */
		static void loadModelBuffer(ModelImporter &modelLoader, std::vector<MeshObject> &modelSet);

		// TODO: add support.
		static void merge(std::vector<MeshObject> &mesh, std::vector<MeshObject> &mesh1);

		/**
		 * @brief
		 */
		static void loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures);
		// static void mergeGeometry(std::vector<ProceduralGeometry::Vertex> wireCubeVertices,
		//						  std::vector<unsigned int> wireCubeIndices);
	};
} // namespace glsample
