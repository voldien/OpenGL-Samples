#ifndef _GL_SAMPLE_SESSION_H_
#define _GL_SAMPLE_SESSION_H_ 1
#include "IOUtil.h"
#include <cxxopts.hpp>

class FVDECLSPEC GLSampleSession {
  public:
	virtual void run() {}
	virtual void commandline(cxxopts::Options &options) {}

	fragcore::IFileSystem *getFileSystem() noexcept { return this->fileSystem; }

  protected:
	fragcore::IFileSystem *fileSystem;
};

#endif