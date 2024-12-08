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
#include <FragCore.h>
#include <GeometryUtil.h>
#include <Math3D/BoundingSphere.h>
#include <Math3D/Plane.h>

namespace glsample {

	// TODO: rename
	class FVDECLSPEC ProcessData {
	  public:
		ProcessData(fragcore::IFileSystem *filesystem);
		virtual ~ProcessData();

		void computeIrradiance(unsigned int env_source, unsigned int &irradiance_target, const unsigned int width,
							   const unsigned int height);
		void computeIrradiance(unsigned int env_source, unsigned int irradiance_target);

		// void computeBump2Normal(unsigned int bump_source, unsigned int& normal_target, const unsigned int width,
		// const unsigned int height);
		// void computeBump2Normal(unsigned int bump_source, unsigned int normal_target);

	  private:
		fragcore::IFileSystem *filesystem;
		int irradiance_program = -1;
		int bump2normal_program = -1;
	};
} // namespace glsample