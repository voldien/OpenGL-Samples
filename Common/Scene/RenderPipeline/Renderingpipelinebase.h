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
#include "GLSampleBase.h"
#include "IRenderPipelineBase.h"
#include <FragCore.h>

namespace glsample {

	/**
	 *	Responsible for rendering scene to default
	 *	framebuffer.
	 */
	class FVDECLSPEC RenderPipelineBase : public IRenderPipelineBase {
	  public:
		void draw(Scene *scene, FrameBuffer *frame) override = 0;

		// void drawCamera(Scene *scene, Camera *camera, FrameBuffer *frame) override;

		/**
		 */
		// virtual void prepareDraw(Scene *scene, Camera *camera );

		/**
		 * Check all lights that has influence inside the camera frustum.
		 */
		// virtual void prepareLight(Scene *scene, Camera *camera );

		/**
		 * TODO rename.

		 */
		// virtual void frustumCullRoot(Node *root, Camera *camera );
		/**		 * Draw scene from camera view.
		 */

		/**
		 *
		 * @param scene
		 * @param camera
		 * @param render
		 */
		// virtual void drawOverlay(Scene * scene, Camera * camera,
		//						 IRenderer * render) = 0;

		/**
		 * Check if light is in camera view.
		 * @param camera
		 * @param light
		 * @return
		 */
		// virtual bool checkLight(Camera* camera, Light* light);		/**		 *		 */		//virtual void
		// sortDrawQueues();

		/**
		 * Draw shadow elements inside camera
		 * frustum.
		 * @param camera
		 */
		// virtual void drawShadow(Scene * scene, Camera * camera,
		//						IRenderer * render) = 0;

		/**
		 * Draw skybox to current view.
		 * @param render
		 * @param scene
		 */
		// virtual void drawSkybox(RenderingInterface *render, Scene *scene);

		/**
		 * Draw GUI elements.
		 * @param scene
		 * @param camera
		 * @param render
		 */
		// virtual void drawGUI(Scene *scene, Camera *camera, RenderingInterface *render);

		/**
		 *	Get rendering queue.
		 */
		//	Queue<Renderer *> &getRenderQueue(RenderQueue queue = Geometry);

		/**
		 *
		 * @return
		 */
		// const Queue<Renderer *> &getRenderQueue(RenderQueue queue = Geometry) const;

		// /**
		//  *	Get main render target.
		//  *	@Return non-null render target.
		//  */
		// inline FrameBufferObject *getRenderTarget() { return this->renderTarget; }

		// /**
		//  *
		//  * @return
		//  */
		// inline PipelineQualitySettings *getQuality() { return this->qualitySettings; }

	  public:
		static void draw(Scene *scene);

	  protected:
		RenderPipelineBase(GLSampleBase &base) : engine(base){};

		GLSampleBase &engine;
		DebugDrawManager *debugDrawer = nullptr;

		/*	*/
		Material *currentBindedMaterial = nullptr;
		Camera *currentActiveCamera = nullptr;

		/*	Scene constant descriptor stuff.	*/

	  protected: /*	Prevent one from creating an instance of this class.	*/
				 /*  */
				 // BufferObject *lightStateBuffer;
				 // BufferObject *shadowStateBuffer;

		// /*  Optimization buffer.    */
		// /*  TODO resolve.   */
		// BufferObject *matricesBuffer;
		// BufferObject *boneBuffer;
		// BufferObject *materialBuffer;

		// /*  */
		// PipelineQualitySettings *qualitySettings;

		// /*  */
		// Queue<Renderer *> *queues; /*  */
		// Queue<Renderer *> rqueue;  /*  Opqaue. */
		// Queue<Renderer *> oqueue;  /*  Translucent.    */
		// Queue<Light *> lqueue;	   /*  Lights. */

		// /*  */
		// Queue<PostEffectObject *> overlay; /*  */
		// std::vector<FrameBufferObject *> shadows;

		// /*  */
		// FrameBufferObject *postTarget;	 /*  */
		// FrameBufferObject *renderTarget; /*  */

		// /*  HDR shader. */
		// ShaderObject *hdr;
		// ShaderObject *gamma;

		/*  */
		//	Geometry* boundbox;
	};
} // namespace glsample
