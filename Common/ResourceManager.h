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
#include <cstdlib>
#include <fmt/format.h>
#include <unistd.h>

namespace glsample {

	/*	*/
	class FVDECLSPEC ResourceManager {
	  public:
		/**
		 * @brief Get the Resource absolute path from
		 * a relative path.
		 */
		static constexpr std::string getResourcePath(const char *relativePath) noexcept {
			/*	Copmute the resource path based on how the software is packaged.	*/
			static bool isAppDir = getenv("APPDIR") != nullptr;
			if (isAppDir) {
				/*	Extract it as if it is installed.	*/
				return fmt::format("{}/usr/lib/asset/{}", getenv("APPDIR"), relativePath);
			} else {
				return fmt::format("{}/asset/{}", ".", relativePath);
			}
		}

		/**
		 * @brief Get the Possible Resource Paths object
		 */
		static std::vector<std::string> getPossibleResourcePaths(const char *relativePath) { return {}; }
	};
} // namespace raylainengine