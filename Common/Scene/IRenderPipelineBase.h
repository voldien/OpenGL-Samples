#pragma once
#include "Core/Object.h"
#include "RenderPipelineSettings.h"
#include "Scene/Scene.h"

// #include <Geometry.h>

namespace glsample {
	using namespace fragcore;
	/**
	 *	Responsible for rendering scene to default
	 *	framebuffer.
	 */
	class FVDECLSPEC IRenderPipelineBase : public Object {
		friend class RenderPipelineFactory;

	  public:
		/**
		 * Draw scene, iterate through each camera
		 * and renderer the scene using the selected
		 * rendering interface in respect to the current settings.
		 */
		virtual void draw(Scene *scene, FrameBuffer *frame) = 0;

		virtual void draw(Camera *camera, FrameBuffer *framebuffer = nullptr);
		virtual void draw(const Light *light);
		virtual void draw(FrameBuffer *framebuffer = nullptr);

		virtual void update(const float deltaTime);

		// virtual void setRenderer(Ref<IRenderer> &renderer) = 0;

		// virtual const Ref<IRenderer> &getRenderer() const = 0;

		// virtual Ref<IRenderer> getRenderer() = 0;

		//	virtual void setSettings(Ref<RenderPipelineSettings> settings);
		//	virtual Ref<RenderPipelineSettings>& getSettings() const;
		//	virtual Ref<RenderPipelineSettings> getSettings();

		// virtual void setViewport(int width, int height, IRenderer *render) = 0;

		virtual RenderQueue getSupportedQueue() const = 0;

	  protected:
		Ref<RenderPipelineSettings> renderQuality;
	};
} // namespace glsample
