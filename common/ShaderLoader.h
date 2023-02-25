/*
* The MIT License (MIT)
* 
* Copyright (c) 2023 Valdemar Lindberg
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
#include <Core/IO/IOUtil.h>
#include <ShaderCompiler.h>

namespace glsample {

	class FVDECLSPEC ShaderLoader {
	  public:
		static int loadGraphicProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,
									  const std::vector<uint32_t> *vertex, const std::vector<uint32_t> *fragment,
									  const std::vector<uint32_t> *geometry = nullptr,
									  const std::vector<uint32_t> *tesselationc = nullptr,
									  const std::vector<uint32_t> *tesselatione = nullptr);
		static int loadGraphicProgram(const std::vector<char> *vertex, const std::vector<char> *fragment,
									  const std::vector<char> *geometry = nullptr,
									  const std::vector<char> *tesselationc = nullptr,
									  const std::vector<char> *tesselatione = nullptr);

		// TODO add support.
		// static int loadComputeProgram(const fragcore::ShaderCompiler::CompilerConvertOption &compilerOptions,const
		// std::vector<std::vector<char> *> &computePaths);
		static int loadComputeProgram(const std::vector<std::vector<char> *> &computePaths);
		static int loadComputeProgram(const std::vector<char> &compute);

	  private:
		static int loadShader(const std::vector<char> &path, int type);
	};
} // namespace glsample