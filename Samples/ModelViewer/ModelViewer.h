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
#include "PBRScene.h"
#include "SampleHelper.h"
#include "Scene/CameraController.h"
#include "Skybox.h"
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <ShaderLoader.h>

namespace glsample {

	/**
	 * @brief
	 */
	class ModelViewer : public GLSampleWindow {
	  public:
		ModelViewer();
		~ModelViewer() override;

		/*	*/
		PBRScene *scene;
		Skybox skybox;

		/*	Image Based Textures.	*/
		unsigned int diffuse_irradiance_cubemap_texture{};
		unsigned int reflection_prefilter_texture{};
		unsigned int brdf_integration_map_texture;

		/*	*/
		unsigned int physical_based_rendering_program{};
		unsigned int simple_physical_based_rendering_program{};
		unsigned int skybox_program{};

		/*  Uniform buffers.    */
		unsigned int uniform_buffer_binding = 0;
		unsigned int uniform_buffer{};
		static const size_t nrUniformBuffer = 3;
		size_t skyboxUniformSize = 0;

		CameraController camera;

		FrameBuffer renderTarget;

		/*	Simple	*/
		const std::string vertexPBRShaderPath = "Shaders/modelviewer/PBR/simplephysicalbasedrendering.vert.spv";
		const std::string fragmentPBRShaderPath = "Shaders/modelviewer/PBR/simplephysicalbasedrendering.frag.spv";

		/*	Advanced.	*/
		const std::string vertexShaderPath = "Shaders/modelviewer/PBR/physicalbasedrendering.vert.spv";
		const std::string fragmentShaderPath = "Shaders/modelviewer/PBR/physicalbasedrendering.frag.spv";
		const std::string ControlShaderPath = "Shaders/modelviewer/PBR/physicalbasedrendering.tesc.spv";
		const std::string EvoluationShaderPath = "Shaders/modelviewer/PBR/physicalbasedrendering.tese.spv";

		class ModelViewerSettingComponent : public GLUIComponent<ModelViewer> {

		  public:
			ModelViewerSettingComponent(ModelViewer &sample) : GLUIComponent(sample) { this->setName("Model Viewer"); }
			void draw() override {

				/*	*/
				this->getRefSample().scene->renderUI();
			}

			bool showWireFrame = false;

		  private:
		};

		std::shared_ptr<ModelViewerSettingComponent> modelviewerSettingComponent;

		void Release() override;

		void Initialize() override;

		void onResize(int width, int height) override;

		void draw() override;
		void update() override;
	};

} // namespace glsample
