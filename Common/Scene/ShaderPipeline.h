#pragma once
#include <FragCore.h>

namespace glsample {

	/*	*/
	class FVDECLSPEC ShaderPipeline : public fragcore::Object {
	  public:
		unsigned int getProgram() const noexcept { return this->program; }

	  private:
		unsigned int program;
	};

	/*	*/
	class FVDECLSPEC GraphicPipeline : public ShaderPipeline {
	  public:
	};

	/*	*/
	class FVDECLSPEC ComputePipeline : public ShaderPipeline {
	  public:
	};
} // namespace glsample
