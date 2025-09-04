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

	/*	*/
	class FVDECLSPEC MiscProcessingUtil {
	  public:
		MiscProcessingUtil(fragcore::IFileSystem *filesystem);
		virtual ~MiscProcessingUtil();

		/*	*/
		void computeDiffuseIrradiance(unsigned int env_texture_panoramic_source, unsigned int &irradiance_target,
									  const unsigned int width, const unsigned int height);
		void computeDiffuseIrradiance(unsigned int env_texture_panoramic_source, unsigned int irradiance_target);

		void computeDiffuseIrradianceCubeMap(unsigned int env_texture_panoramic_source,
											 unsigned int& irradiance_cubemap_texture_target, unsigned int width,
											 unsigned int height);
		void computeDiffuseIrradianceCubeMap(unsigned int env_texture_panoramic_source,
											 unsigned int& irradiance_cubemap_texture_target);

		/*	*/
		void computeReflectanceIrradiance(unsigned int env_texture_panoramic_source, unsigned int &irradiance_target,
										  const unsigned int width, const unsigned int height);
		void computeReflectanceIrradiance(unsigned int env_texture_panoramic_source, unsigned int irradiance_target);
		void computeReflectanceIrradianceCubeMap(unsigned int env_texture_panoramic_source,
											 unsigned int& irradiance_cubemap_texture_target, unsigned int width,
											 unsigned int height);
		void computeReflectanceIrradianceCubeMap(unsigned int env_texture_panoramic_source,
											 unsigned int& irradiance_cubemap_texture_target);

		/*	*/
		void computeBRDFIntegrationMap(unsigned int &brdf_integration_target_texture, const unsigned int width,
									   const unsigned int height);
		void computeBRDFIntegrationMap(unsigned int brdf_integration_target_texture);

		/*	*/
		void computePerlinNoise(unsigned int *target, const unsigned int width, const unsigned int height,
								const glm::vec2 &size = glm::vec2(10, 10),
								const glm::vec2 &tile_offset = glm::vec2(10, 10), const int octaves = 16);
		void computePerlinNoise(unsigned int target, const glm::vec2 &size = glm::vec2(10, 10),
								const glm::vec2 &tile_offset = glm::vec2(10, 10), const int octaves = 16);

		/*	*/
		void computeBump2Normal(unsigned int bump_source_texture, unsigned int &normal_texture_target,
								const unsigned int width, const unsigned int height);
		void computeBump2Normal(unsigned int bump_source_texture, unsigned int normal_texture_target);

		/*	*/
		void computeColor2HeightMap(unsigned int color_source, unsigned int &height_target, const unsigned int width,
									const unsigned int height);
		void computeColor2HeightMap(unsigned int color_source, unsigned int height_target);

	  private:
		fragcore::IFileSystem *filesystem = nullptr;
		int irradiance_diffuse_program = -1;
		int irradiance_diffuse_cubemap_program = -1;
		int irradiance_specular_program = -1;
		int irradiance_specular_cubemap_program = -1;
		int brdf_integration_map_program = -1;
		int bump2normal_program = -1;
		int perlin_noise2D_program = -1;
		//			std::array<int, (size_t)ColorSpace::MaxColorSpaces * 3> compute_programs_local_workgroup_sizes{};
	};
} // namespace glsample
