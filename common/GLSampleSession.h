/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Valdemar Lindberg
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
#include "SampleHelper.h"
#include "TaskScheduler/IScheduler.h"
#include <FragCore.h>
#include <GL/glew.h>
#include <cxxopts.hpp>

namespace glsample {

	// TODO relocate.
	using DrawElementsIndirectCommand = struct {
		GLuint count;
		GLuint instanceCount;
		GLuint firstIndex;
		GLuint baseVertex;
		GLuint baseInstance;
	};

	using DrawArraysIndirectCommand = struct {
		GLuint count;
		GLuint instanceCount;
		GLuint first;
		GLuint baseInstance;
	};

	using MeshObject = struct geometry_object_t {
		/*	*/
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ibo = 0;

		size_t nrIndicesElements = 0;
		size_t nrVertices = 0;

		size_t vertex_offset = 0;
		size_t indices_offset = 0;

		unsigned int stride = 0;
		int primitiveType = 0;

		/*	*/
		fragcore::Bound bound{};
	};

	using TextureObject = struct texture_object_t {
		unsigned int width{};
		unsigned int height{};
		unsigned int depth{};
		unsigned int texture = 0;
	};

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC GLSampleSession {
	  public:
		virtual ~GLSampleSession() = default;
		virtual void run(int argc, const char **argv, const std::vector<const char *> &requiredExtension = {}) = 0;
		virtual void customOptions(cxxopts::OptionAdder &options) {}

		fragcore::IFileSystem *getFileSystem() noexcept { return this->activeFileSystem; }
		fragcore::IScheduler *getSchedular() noexcept { return this->schedular; }

	  protected:
		fragcore::IFileSystem *activeFileSystem{};
		fragcore::IScheduler *schedular{};
	};
} // namespace glsample