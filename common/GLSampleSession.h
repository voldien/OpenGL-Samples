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
#include "IOUtil.h"
#include <cxxopts.hpp>

namespace glsample {

	// TODO relocate.
	typedef struct geometry_object_t {
		unsigned int vao;
		unsigned int vbo;
		unsigned int ibo;
		size_t nrIndicesElements = 0;
		size_t nrVertices = 0;

		size_t vertex_offset = 0;
		size_t indices_offset = 0;
	} GeometryObject;

	typedef struct texture_object_t {
		unsigned int width;
		unsigned int height;
		unsigned int depth;
		unsigned int texture;
	} TextureObject;

	class FVDECLSPEC GLSampleSession {
	  public:
		virtual void run(int argc, const char **argv, const std::vector<const char *> &requiredExtension = {}) = 0;
		virtual void customOptions(cxxopts::OptionAdder &options) {}

		fragcore::IFileSystem *getFileSystem() noexcept { return this->activeFileSystem; }

	  protected:
		fragcore::IFileSystem *activeFileSystem;
	};
} // namespace glsample