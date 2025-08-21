#include "ModelViewer.h"

using namespace glsample;

// PBRScene::PBRScene() { /*  */ }

void PBRScene::init() {
	Scene::init();
	this->stageLightData.getBase()->directionalCount = 1;
}

void PBRScene::bindMaterial(const MaterialObject *material) {
	Scene::bindMaterial(material);
	if (shadowPass) {
		/*	*/
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}
