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
		template <typename T = glsample::Scene> static T loadFrom(ModelImporter &importer) {

			T scene;

			scene.init(importer.getFileSystem());

			/*	*/
			scene.nodes = importer.getNodes();

			ImportHelper::loadModelBuffer(importer, scene.refGeometry);
			ImportHelper::loadTextures(importer, scene.refTexture);

			convertNodeSystem(scene, importer);

			scene.materials = importer.getMaterials();

			convertLightSystem(scene, importer);
			convertAnimationSystem(scene, importer);
			convertMaterialSystem(scene, importer);

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
					point->setColor(lights[i].mColorDiffuse);

					scene.getLights().push_back(point);
				} break;
				default:
				case 1: {
					DirectionalLight *direction = new DirectionalLight();
					direction->setPosition(lights[i].position);
					direction->rotateTowards(lights[i].direction);
					direction->setColor(lights[i].mColorDiffuse);
					scene.getLights().push_back(direction);
				} break;

					break;
				}

				// lights[i].
			}
		}

		static void convertMaterialSystem(Scene &scene, const ModelImporter &importer) {
			// scene.getMaterials().resize(importer.getMaterials().size());
			for (size_t i = 0; i < importer.getMaterials().size(); i++) {
				const MaterialObject &mat = importer.getMaterials()[i];

				Material material;
			//	material.
			}
		}

		static void convertAnimationSystem(Scene &scene, const ModelImporter &importer) {
			for (size_t i = 0; i < importer.getAnimation().size(); i++) {
				const AnimationObject &anim = importer.getAnimation()[i];

				/*	Transfer animation data.	*/
				AnimationPlayer *animationPlayer = new AnimationPlayer();
				animationPlayer->time = anim.duration;
				animationPlayer->setName(anim.name);
				animationPlayer->curves = anim.curves_s;

				/*	*/
				scene.getAnimation().push_back(animationPlayer);
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
