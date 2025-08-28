#pragma once
#include <FragCore.h>

namespace glsample {

	class FVDECLSPEC ShaderPipeline : public fragcore::Object {
	  public:
	};

	class FVDECLSPEC GraphicPipeline : public ShaderPipeline {
	  public:
	};

	class FVDECLSPEC ComputePipeline : public ShaderPipeline {
	  public:
	};
} // namespace glsample
