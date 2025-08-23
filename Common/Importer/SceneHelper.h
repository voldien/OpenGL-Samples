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
#include "ImportHelper.h"
#include "ModelImporter.h"
#include "Node.h"
#include "Scene.h"

namespace glsample {

	class FVDECLSPEC SceneHelper {
	  public:
		template <typename T = glsample::Scene> static T loadFrom(ModelImporter &importer) {

			T scene;

			/*	*/
			scene.nodes = importer.getNodes();

			ImportHelper::loadModelBuffer(importer, scene.refGeometry);
			ImportHelper::loadTextures(importer, scene.refTexture);

			convertNodeSystem(scene, importer);

			scene.materials = importer.getMaterials();

			convertLightSystem(scene, importer);

			return scene;
		}

		static void convertLightSystem(Scene &scene, const ModelImporter &importer) {

			/*	*/
			const std::vector<LightObject> &lights = importer.getLights();

			for (size_t i = 0; i < lights.size(); i++) {

				switch (lights[i].type) {

				case 2: {
					PointLight *point = new PointLight();
					point->setPosition(lights[i].position);

					scene.getLights().push_back(point);
				} break;
				default:
				case 1: {
					DirectionalLight *direction = new DirectionalLight();
					direction->setPosition(lights[i].position);
					// direction->setRotation(GLM2E((lights[i].direction * glm::vec3(0,0,1)) ));
					scene.getLights().push_back(direction);
				} break;

					break;
				}

				// lights[i].
			}
		}

		static void convertNodeSystem(Scene &scene, ModelImporter &importer) {

			for (size_t node_index = 0; node_index < importer.getNodes().size(); node_index++) {
				NodeObject *node_Obj = importer.getNodes()[node_index];
				Node *node = new Node();

				node->geometryObjectIndex = node_Obj->geometryObjectIndex;
				node->materialIndex = node_Obj->materialIndex;

				convertNodeChildren(node_Obj, node);
			}
		}

		static void convertNodeChildren(NodeObject *node0, Node *node) {}
	};

}; // namespace glsample
