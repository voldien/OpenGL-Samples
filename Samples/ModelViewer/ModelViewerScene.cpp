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
#include <IO/IOUtil.h>
#include <ShaderLoader.h>
#include <glm/fwd.hpp>

using namespace glsample;

// PBRScene::PBRScene() { /*  */ }

void PBRScene::init(IFileSystem *filesystem) {

	const std::string vertexShadowShaderPath = "Shaders/scene/shadow/pointlightshadow.vert.spv";
	const std::string geomtryShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.geom.spv";
	const std::string fragmentShadowShaderPath = "Shaders/shadowpointlight/pointlightshadow.frag.spv";
	const std::string fragmentShadowAlphaClipShaderPath =
		"Shaders/shadowpointlight/pointlightshadow_alphaclip.frag.spv";

	/*	*/
	const std::string vertexDirectionalShadowShaderPath = "Shaders/scene/shadow/scene_directional_shadow.vert.spv";
	const std::string fragmentDirectionalShadowShaderPath = "Shaders/scene/shadow/scene_directional_shadow.frag.spv";

	/*	*/
	const std::vector<uint32_t> vertex_shadow_binary =
		fragcore::IOUtil::readFileData<uint32_t>(vertexDirectionalShadowShaderPath, filesystem);
	const std::vector<uint32_t> fragment_shadow_binary =
		fragcore::IOUtil::readFileData<uint32_t>(fragmentDirectionalShadowShaderPath, filesystem);
	// const std::vector<uint32_t> fragment_shadow_alpha_binary =
	// 	IOUtil::readFileData<uint32_t>(this->fragmentClippingShadowShaderPath, filesystem);

	fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
	compilerOptions.target = fragcore::ShaderLanguage::GLSL;
	compilerOptions.glslVersion = 420;

	/*	Load shaders	*/
	// this->graphic_program =
	//	ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_binary);
	// this->graphic_pfc_program =
	//	ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_graphic_binary, &fragment_graphic_pfc_binary);
	this->shadow_directional =
		ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_shadow_binary, &fragment_shadow_binary);
	// this->shadow_alpha_clip_program =
	//	ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_shadow_binary, &fragment_shadow_alpha_binary);
}

void PBRScene::init() {
	Scene::init();

	/*	*/
	this->getLights().push_back(new DirectionalLight());
}

void PBRScene::shadowPass() {

	UseShadowPass = true;
	glUseProgram(this->shadow_directional);
	for (size_t i = 0; i < this->getLights().size(); i++) {
		Light *light = getLights()[i];

		if (light->getShadowStrength() > 0 && light->getFrameBuffer()) {

			FrameBuffer *framebuffer = light->getFrameBuffer();
			const glm::ivec3 size = light->getSize();

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

			glClear(GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, size[0], size[1]);
			Scene::render(light);
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
