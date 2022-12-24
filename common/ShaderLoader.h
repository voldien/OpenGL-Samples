#pragma once
#include <Core/IO/IOUtil.h>
#include <ShaderCompiler.h>

namespace glsample {

	class FVDECLSPEC ShaderLoader {
	  public:
		static int loadGraphicProgram(const std::vector<char> *vertex, const std::vector<char> *fragment,
									  const std::vector<char> *geometry = nullptr,
									  const std::vector<char> *tesselationc = nullptr,
									  const std::vector<char> *tesselatione = nullptr);

		static int loadComputeProgram(const std::vector<std::vector<char> *> &computePaths);
		static int loadComputeProgram(const std::vector<char> &compute);

	  private:
		static int loadShader(const std::vector<char> &path, int type);
	};
} // namespace glsample