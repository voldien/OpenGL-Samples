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

#include <Exception.hpp>
#include <IO/FileSystem.h>
#include <IO/IOUtil.h>
#include <fmt/format.h>
#include <vector>

namespace glsample {
	using namespace fragcore;

	/**
	 * @brief
	 *
	 */
	class FVDECLSPEC IOUtil {
	  public:
		static std::vector<char> readFileString(const std::string &filename, IFileSystem *filesystem) {

			Ref<IO> ref = Ref<IO>(filesystem->openFile(filename.c_str(), IO::IOMode::READ));
			std::vector<char> string = fragcore::IOUtil::readString<char>(ref);
			ref->close();
			return string;
		}

		static std::vector<char> readFileString(const std::string &filename) {
			return glsample::IOUtil::readFileString(filename, FileSystem::getFileSystem());
		}

		template <typename T> static std::vector<T> readFileData(const std::string &filename, IFileSystem *filesystem) {

			Ref<IO> ref = Ref<IO>(filesystem->openFile(filename.c_str(), IO::IOMode::READ));
			std::vector<T> buffer = fragcore::IOUtil::readFileData<T>(ref);
			ref->close();
			return buffer;
		}
	};
} // namespace glsample
