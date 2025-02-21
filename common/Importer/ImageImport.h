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
#include "../Common.h"
#include <IO/FileSystem.h>
#include <IO/IFileSystem.h>
#include <ImageLoader.h>
#include <string>
#include <vector>

namespace glsample {

	enum class TextureCompression {
		None,	 /*	*/
		Default, /*	*/
		BPTC,	 /*	*/
		ASTC	 /*	*/
	};

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC TextureImporter {
	  public:
		TextureImporter(const TextureImporter &) = delete;
		TextureImporter(TextureImporter &&) = delete;
		TextureImporter &operator=(const TextureImporter &) = delete;
		TextureImporter &operator=(TextureImporter &&) = delete;
		TextureImporter(fragcore::IFileSystem *filesystem);
		virtual ~TextureImporter();

		int loadImage2D(const std::string &path, const ColorSpace colorSpace = ColorSpace::RawLinear,
						const TextureCompression compression = TextureCompression::None);
		int loadImage2DRaw(const fragcore::Image &image, const ColorSpace colorSpace = ColorSpace::RawLinear,
						   const TextureCompression compression = TextureCompression::None);

		int loadCubeMap(const std::string &px, const std::string &nx, const std::string &py, const std::string &ny,
						const std::string &pz, const std::string &nz,
						const ColorSpace colorSpace = ColorSpace::RawLinear,
						const TextureCompression compression = TextureCompression::None);
		int loadCubeMap(const std::vector<std::string> &paths, const ColorSpace colorSpace = ColorSpace::RawLinear,
						const TextureCompression compression = TextureCompression::None);

	  private:
		fragcore::IFileSystem *filesystem = nullptr;
		std::array<unsigned int, 3> pbos = {};
		std::atomic_int current_aviable = 0;
	};

} // namespace glsample