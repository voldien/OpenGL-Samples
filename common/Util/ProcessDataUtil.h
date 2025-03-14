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
#include <FragCore.h>
#include <GeometryUtil.h>
#include <Math3D/BoundingSphere.h>
#include <Math3D/Plane.h>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

namespace glsample {

	// TODO: rename
	class FVDECLSPEC ProcessData {
	  public:
		ProcessData(fragcore::IFileSystem *filesystem);
		virtual ~ProcessData();

		/**/
		void computeIrradiance(unsigned int env_source, unsigned int &irradiance_target, const unsigned int width,
							   const unsigned int height);
		void computeIrradiance(unsigned int env_source, unsigned int irradiance_target);

		/*	*/
		void computePerlinNoise(unsigned int *target, const unsigned int width, const unsigned int height,
								const glm::vec2 &size = glm::vec2(10, 10),
								const glm::vec2 &tile_offset = glm::vec2(10, 10), const int octaves = 16);
		void computePerlinNoise(unsigned int target, const glm::vec2 &size = glm::vec2(10, 10),
								const glm::vec2 &tile_offset = glm::vec2(10, 10), const int octaves = 16);
		/**/
		void computeBump2Normal(unsigned int bump_source, unsigned int &normal_target, const unsigned int width,
								const unsigned int height);
		void computeBump2Normal(unsigned int bump_source, unsigned int normal_target);

	  private:
		fragcore::IFileSystem *filesystem = nullptr;
		int irradiance_program = -1;
		int bump2normal_program = -1;
		int perlin_noise2D_program = -1;
		//			std::array<int, (size_t)ColorSpace::MaxColorSpaces * 3> compute_programs_local_workgroup_sizes{};
	};
} // namespace glsample