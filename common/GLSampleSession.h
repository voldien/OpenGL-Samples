#ifndef _GL_SAMPLE_SESSION_H_
#define _GL_SAMPLE_SESSION_H_ 1
#include "IOUtil.h"
#include <cxxopts.hpp>

// TODO relocate.
typedef struct geometry_object_t {
	unsigned int vao;
	unsigned int vbo;
	unsigned int ibo;
	unsigned int nrIndicesElements;
	unsigned int nrVertices;

	unsigned int vertex_offset;
	unsigned int indices_offset;
} GeometryObject;

class FVDECLSPEC GLSampleSession {
  public:
	virtual void run() {}
	virtual void commandline(cxxopts::Options &options) {}

	fragcore::IFileSystem *getFileSystem() noexcept { return this->activeFileSystem; }

  protected:
	fragcore::IFileSystem *activeFileSystem;
};

#endif