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
#include "Scene/RenderQueue.h"
#include <IO/IOUtil.h>
#include <ShaderLoader.h>
#include <glm/fwd.hpp>

using namespace glsample;

// PBRScene::PBRScene() { /*  */ }

void PBRScene::init(IFileSystem *filesystem) {
	Scene::init(filesystem);

	/*	*/
	DirectionalLight *dirLight = new DirectionalLight();
	dirLight->setSize(glm::ivec3(1024, 1024, 1));
	this->getLights().push_back(dirLight);

	/*	Point Light.	*/
	const std::string vertexPointShadowShaderPath = "Shaders/scene/shadow/scene_point_shadow.vert.spv";
	const std::string geomtryPointShadowShaderPath = "Shaders/scene/shadow/scene_point_shadow.geom.spv";
	const std::string fragmentPointShadowShaderPath = "Shaders/scene/shadow/scene_point_shadow.frag.spv";
	const std::string fragmentPointShadowAlphaClipShaderPath = "Shaders/scene/shadow/scene_point_shadow_alpha.frag.spv";

	/*	Directional Light */
	const std::string vertexDirectionalShadowShaderPath = "Shaders/scene/shadow/scene_directional_shadow.vert.spv";
	const std::string fragmentDirectionalShadowShaderPath = "Shaders/scene/shadow/scene_directional_shadow.frag.spv";
	const std::string fragmentDirectionalShadowAlphaShaderPath =
		"Shaders/scene/shadow/scene_directional_shadow_alpha.frag.spv";

	/*	*/
	fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
	compilerOptions.target = fragcore::ShaderLanguage::GLSL;
	compilerOptions.glslVersion = 330;

	/*	*/
	const std::vector<uint32_t> vertex_point_shadow_binary =
		IOUtil::readFileData<uint32_t>(vertexPointShadowShaderPath, filesystem);
	const std::vector<uint32_t> geometry_point_shadow_binary =
		IOUtil::readFileData<uint32_t>(geomtryPointShadowShaderPath, filesystem);
	const std::vector<uint32_t> fragment_point_shadow_binary =
		IOUtil::readFileData<uint32_t>(fragmentPointShadowShaderPath, filesystem);
	const std::vector<uint32_t> fragment_point_shadow_alpha_clip_binary =
		IOUtil::readFileData<uint32_t>(fragmentPointShadowAlphaClipShaderPath, filesystem);

	this->shadow_point = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_point_shadow_binary,
														  &fragment_point_shadow_binary, &geometry_point_shadow_binary);
	this->shadow_point_alpha =
		ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_point_shadow_binary,
										 &fragment_point_shadow_alpha_clip_binary, &geometry_point_shadow_binary);

	/*	*/
	const std::vector<uint32_t> vertex_directional_shadow_binary =
		fragcore::IOUtil::readFileData<uint32_t>(vertexDirectionalShadowShaderPath, filesystem);
	const std::vector<uint32_t> fragment_directional_shadow_binary =
		fragcore::IOUtil::readFileData<uint32_t>(fragmentDirectionalShadowShaderPath, filesystem);
	const std::vector<uint32_t> fragment_directional_shadow_alpha_binary =
		fragcore::IOUtil::readFileData<uint32_t>(fragmentDirectionalShadowAlphaShaderPath, filesystem);

	this->shadow_directional = ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_directional_shadow_binary,
																&fragment_directional_shadow_binary);
	this->shadow_directional_alpha = ShaderLoader::loadGraphicProgram(
		compilerOptions, &vertex_directional_shadow_binary, &fragment_directional_shadow_alpha_binary);

	/*	Convert material to be more PBR ready.	*/
	for (size_t i = 0; i < getMaterials().size(); i++) {
		Material *mat = &getMaterials()[i];
		if (mat->shinininess > 1) {
			mat->shinininess *= (1.0f / 32.0f);
		}
		//mat->specular = glm::vec4(1, 1, 1, 1);
	}
}

void PBRScene::shadowPass() {

	/*	*/
	GLint currentDrawFBO;
	GLint currentReadFBO;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentDrawFBO);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentReadFBO);

	/*	*/
	this->UseShadowPass = true;

	/*	*/
	for (size_t i = 0; i < this->getLights().size(); i++) {
		const Light *light = getLights()[i];

		if (light->isActive() && light->getShadowStrength() > 0 && light->getFrameBuffer()) {

			FrameBuffer *framebuffer = light->getFrameBuffer();
			const glm::ivec3 size = light->getSize();

			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->framebuffer);

			glClear(GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, size[0], size[1]);
			Scene::render(light);
		}
	}

	/*	Restore framebuffer state.	*/
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentDrawFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);

	this->UseShadowPass = false;
}

void PBRScene::render(Camera *camera, FrameBuffer *framebuffer) {

	/*	Render all light sources.	*/
	{
		if (camera) {
			this->currentActiveCamera = camera;
		}
		this->shadowPass();
	}

	/*	*/
	Scene::render(camera, framebuffer);
}

void PBRScene::render(FrameBuffer *framebuffer) {

	for (size_t i = 0; i < this->getLights().size(); i++) {

		FrameBuffer *frame = this->getLights()[i]->getFrameBuffer();

		if (frame) {

			glActiveTexture(GL_TEXTURE0 + DirectionalLightDepthBuffer + i);
			glBindTexture(GL_TEXTURE_2D, frame->attachments[frame->depthIndex]);
		}
	}

	Scene::render(framebuffer);
}

void PBRScene::bindMaterial(const Material *material) {
	Scene::bindMaterial(material);

	if (UseShadowPass) {

		RenderQueue queue = getQueueDomain(*material);
		if (queue >= RenderQueue::AlphaTest) {
			glUseProgram(this->shadow_directional_alpha);
		} else {
			glUseProgram(this->shadow_directional);
		}

		/*	*/
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(GL_TRUE); /*		Allow Depth to written.	*/
		if (material->culling_both_side_mode) {
			glCullFace(GL_FRONT_AND_BACK);
		} else {
			glCullFace(GL_FRONT);
		}

		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}
