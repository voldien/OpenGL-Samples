#pragma once
#include "IOUtil.h"
#include <cxxopts.hpp>

// TODO relocate.
typedef struct geometry_object_t {
	unsigned int vao;
	unsigned int vbo;
	unsigned int ibo;
	size_t nrIndicesElements;
	size_t nrVertices;

	size_t vertex_offset;
	size_t indices_offset;
} GeometryObject;

namespace glsample {

	class FVDECLSPEC GLSampleSession {
	  public:
		virtual void run(int argc, const char **argv) = 0;
		virtual void customOptions(cxxopts::OptionAdder &options) {}

		fragcore::IFileSystem *getFileSystem() noexcept { return this->activeFileSystem; }

	  protected:
		fragcore::IFileSystem *activeFileSystem;
	};
} // namespace glsample