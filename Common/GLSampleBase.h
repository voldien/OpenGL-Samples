/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2025 Valdemar Lindberg
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
#include "FragDef.h"
#include "GLRendererInterface.h"
#include "MIMIWindow.h"
#include "SampleHelper.h"
#include "cxxopts.hpp"
#include <string>

/*	*/
class FVDECLSPEC GLSampleBase : public nekomimi::MIMIWindow {
  public:
	GLSampleBase();
	~GLSampleBase() override;

	/*	*/
	const cxxopts::ParseResult &getResult() const noexcept { return this->parseResult; }

	std::string getResourcePath(const char *relativePath) const noexcept;

	fragcore::IFileSystem *getFileSystem() const noexcept { return this->filesystem.get(); }
	void setFileSystem(fragcore::IFileSystem *filesystem) noexcept {
		this->filesystem = std::shared_ptr<fragcore::IFileSystem>(filesystem);
	}

	fragcore::IScheduler *getSchedular() const noexcept { return this->filesystem->getScheduler().get(); }

	unsigned int getShaderVersion() const;
	bool supportSPIRV() const;

	const fragcore::GLRendererInterface *getGLRenderInterface() const noexcept {
		return &this->getRenderInterface()->as<const fragcore::GLRendererInterface>();
	}
	fragcore::GLRendererInterface *getGLRenderInterface() noexcept {
		return &this->getRenderInterface()->as<fragcore::GLRendererInterface>();
	}

	const glsample::UBOPool &getUniformPool() const noexcept { return this->uniformPool; }
	glsample::UBOPool &getUniformPool() noexcept { return this->uniformPool; }

  protected:
	cxxopts::ParseResult parseResult;
	glsample::UBOPool uniformPool;
	std::shared_ptr<fragcore::IFileSystem> filesystem; /*	*/
};
