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
#include "ModelImporter.h"

namespace glsample {

	class FVDECLSPEC ImportHelper {
	  public:
		/**
		 * @brief
		 *
		 * @param modelLoader
		 * @param modelSet
		 */
		static void loadModelBuffer(ModelImporter &modelLoader, std::vector<MeshObject> &modelSet);

		/**
		 * @brief
		 *
		 * @param modelLoader
		 * @param textures
		 */
		static void loadTextures(ModelImporter &modelLoader, std::vector<TextureAssetObject> &textures);

		// static void mergeGeometry(std::vector<ProceduralGeometry::Vertex> wireCubeVertices,
		//						  std::vector<unsigned int> wireCubeIndices);
	};
} // namespace glsample