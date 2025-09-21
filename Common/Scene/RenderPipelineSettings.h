#pragma once
#include <Core/IConfig.h>
#include <Core/Ref.h>
#include <RenderPrerequisites.h>

namespace fragcore {
	/**
	 *
	 */
	class FVDECLSPEC RenderPipelineSettings {
	  public:
		RenderPipelineSettings(const IConfig &other);
		/*  TODO add memory mapping and etc.    */
	  private:
		// virtual IConfig *getSuperInstance() override;

	  protected:
		RenderPipelineSettings();

	  protected:
		Ref<Buffer> buffer;
	};
} // namespace fragcore
