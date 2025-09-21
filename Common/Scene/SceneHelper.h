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

#include "FragDef.h"
#include "Importer/ImportHelper.h"
#include "Importer/ModelImporter.h"
#include "Node.h"
#include "Scene.h"

namespace glsample {

	class FVDECLSPEC SceneHelper {
	  public:
		template <typename T = glsample::Scene> static T *loadFrom(ModelImporter &importer) {

			T *scene = new T();

			scene->init(importer.getFileSystem());

			// TODO: conditional.
			/*	*/
			ImportHelper::loadModelBuffer(importer, scene->refGeometry);
			ImportHelper::loadTextures(importer, scene->refTexture);

			convertNodeSystem(*scene, importer);

			convertLightSystem(*scene, importer);
			convertAnimationSystem(*scene, importer);
			convertMaterialSystem(*scene, importer);

			return scene;
		}

		static void convertLightSystem(Scene &scene, const ModelImporter &importer);
		static void convertCameraSystem(Scene &scene, const ModelImporter &importer);
		static void convertMaterialSystem(Scene &scene, const ModelImporter &importer);
		static void convertAnimationSystem(Scene &scene, const ModelImporter &importer);

		static void convertNodeSystem(Scene &scene, ModelImporter &importer);
		static void convertNodeChildren(const NodeObject *node0, Node *node);
	};

}; // namespace glsample
