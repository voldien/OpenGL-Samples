#pragma once
#include "GLSampleBase.h"
#include "Renderingpipelinebase.h"

// TOOD rename namespace
namespace glsample {
	using namespace fragcore;

	/**
	 *	Responsible for rendering scene to default
	 *	framebuffer.
	 */
	class FVDECLSPEC RenderPipelineForward : public RenderPipelineBase {
	  public:
		void draw(Scene *scene, FrameBuffer *frame) override;
		//
		//	void setViewport(int width, int height ) override;

		//	/**
		//	 *	Draw scene from camera view.
		//	 *
		//	 */
		//	virtual void drawCamera(Scene * scene,
		//	                        Camera * camera,
		// RenderQueue getSupportedQueue() const override;

		//	void drawCamera(Scene *scene, Camera *camera, IRenderer *render) override;
		//	                        IRenderer * render);
		//
		//	/**
		//	 *
		//	 */
		//	virtual void drawShadow(Scene *scene, Camera *camera, IRenderer *render);
		//
		//	virtual void drawOverlay(Scene*  scene,
		//	                         Camera* camera,
		//	                         IRenderer* render);

	  public:
		RenderPipelineForward(GLSampleBase &base);
		~RenderPipelineForward() override;

	  protected: /*	Prevent one from creating an instance of this class.	*/
	};

} // namespace glsample
