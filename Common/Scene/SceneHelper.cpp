#include "SceneHelper.h"

using namespace glsample;

void SceneHelper::convertLightSystem(Scene &scene, const ModelImporter &importer) {

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

void SceneHelper::convertMaterialSystem(Scene &scene, const ModelImporter &importer) {
	// scene.getMaterials().resize(importer.getMaterials().size());
	for (size_t i = 0; i < importer.getMaterials().size(); i++) {
		const MaterialObject &mat = importer.getMaterials()[i];

		Material material;
		//	material.
	}
}

void SceneHelper::convertAnimationSystem(Scene &scene, const ModelImporter &importer) {
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

void SceneHelper::convertNodeSystem(Scene &scene, ModelImporter &importer) {

    /*  */
    importer.getNodeRoot();

    
	for (size_t node_index = 0; node_index < importer.getNodes().size(); node_index++) {

		NodeObject *node_Obj = importer.getNodes()[node_index];
		Node *node = new Node(); // TODO: get from pool.

		node->setPosition(node_Obj->localPosition);
		node->setScale(node_Obj->localScale);
		node->setRotation(node_Obj->localRotation);

		node->modelLocalTransform = node_Obj->modelLocalTransform;
		node->modelGlobalTransform = node_Obj->modelGlobalTransform;

		node->geometryObjectIndex = node_Obj->geometryObjectIndex;
		node->materialIndex = node_Obj->materialIndex;

		convertNodeChildren(node_Obj, node);
	}
}

void SceneHelper::convertNodeChildren(NodeObject *node0, Node *node) {}