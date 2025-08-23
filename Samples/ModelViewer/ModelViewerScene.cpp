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
#include "PBRScene.h"
#include "SampleHelper.h"
#include "Scene/Light.h"
#include <glm/fwd.hpp>

using namespace glsample;

// PBRScene::PBRScene() { /*  */ }

void PBRScene::init() {
	Scene::init();

	/*	*/
	this->getLights().push_back(new DirectionalLight());
}

void PBRScene::shadowPass() {
	UseShadowPass = true;
	for (size_t i = 0; i < this->getLights().size(); i++) {
		Light *light = getLights()[i];

		if (light->getShadowStrength() > 0 && light->getFrameBuffer()) {

			FrameBuffer *framebuffer = light->getFrameBuffer();
			const glm::ivec3 size = light->getSize();

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

			glClear(GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, size[0], size[1]);
			Scene::render(light);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}
	UseShadowPass = false;
}

void PBRScene::render(Camera *camera) { Scene::render(camera); }
void PBRScene::render() {

	for (size_t i = 0; i < this->getLights().size(); i++) {
		FrameBuffer *frame = this->getLights()[i]->getFrameBuffer();

		if (frame) {

			 	glActiveTexture(GL_TEXTURE0 + DirectionalLightDepthBuffer + i);
			 	glBindTexture(GL_TEXTURE_2D, frame->attachments[frame->depthIndex]);
		}
	}

	Scene::render();
}

void PBRScene::bindMaterial(const MaterialObject *material) {
	Scene::bindMaterial(material);
	if (UseShadowPass) {
		/*	*/
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}
