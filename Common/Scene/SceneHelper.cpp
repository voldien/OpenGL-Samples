#include "SceneHelper.h"
#include "Importer/ModelImporter.h"
#include "Scene/Node.h"

using namespace glsample;

void SceneHelper::convertLightSystem(Scene &scene, const ModelImporter &importer) {

	/*	*/
	const std::vector<LightObject> &lights = importer.getLights();

	for (size_t i = 0; i < lights.size(); i++) {

		switch (lights[i].type) {

		case 2: {
			PointLight *point = new PointLight();
			point->setName(lights[i].name);
			point->setPosition(lights[i].position);
			point->setColor(lights[i].mColorDiffuse);

			scene.getLights().push_back(point);
		} break;
		default:
		case 1: {
			DirectionalLight *direction = new DirectionalLight();
			direction->setName(lights[i].name);

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
void SceneHelper::convertCameraSystem(Scene &scene, const ModelImporter &importer) {

	for (size_t i = 0; i < importer.getCameras().size(); i++) {
		const CameraData &camera = importer.getCameras()[i];
		scene.getCameras().push_back(new Camera());
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
	const size_t nr_nodes = importer.getNodes().size();
	scene.nodePool = std::vector<Node>(nr_nodes);

	/*	Convert nodes.	*/
	for (size_t node_index = 0; node_index < nr_nodes; node_index++) {

		/*	*/
		const NodeObject *node_Obj = importer.getNodes()[node_index];
		Node *node = &scene.nodePool[node_index];

		/*	*/
		const int node_parent_index = node_Obj->parent_index;
		assert(node_parent_index != node_index);
		if (node_parent_index >= 0) {
			Node *parent = &scene.nodePool[node_parent_index];
			parent->addChild(node);
		}

		/*	*/
		convertNodeChildren(node_Obj, node);
	}

	scene.getNodes().resize(nr_nodes);
	for (size_t node_index = 0; node_index < nr_nodes; node_index++) {
		scene.getNodes()[node_index] = &scene.nodePool[node_index];
	}
}

void SceneHelper::convertNodeChildren(const NodeObject *node_Obj, Node *node) {
	node->setName(node_Obj->name);

	node->setGlobalPositionDirect(node_Obj->globalPosition);
	node->setGlobalScaleDirect(node_Obj->globalScale);
	node->setGlobalRotationDirect(node_Obj->globalRotation);

	node->bound = node_Obj->bound;

	node->modelLocalTransform = node_Obj->modelLocalTransform;
	node->modelGlobalTransform = node_Obj->modelGlobalTransform;

	node->geometryObjectIndex = node_Obj->geometryObjectIndex;
	node->materialIndex = node_Obj->materialIndex;
}