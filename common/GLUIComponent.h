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
#include "GLSampleWindow.h"
#include <UIComponent.h>

namespace glsample {

	template <typename T = GLSampleWindow> class GLUIComponent : public nekomimi::UIComponent {
	  public:
		GLUIComponent(T &base, const std::string &name = "Sample Settings") : base(base) { this->setName(name); }

		void draw() override {}

	  protected:
		T &getRefSample() const { return this->base; }

	  private:
		T &base;
	};
} // namespace glsample