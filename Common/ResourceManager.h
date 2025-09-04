#pragma once

#include "fmt/format.h"
#include <string>


class ResourceManager {

	std::string getResourcePath(const char *relativePath) {

		/*	Copmute the resource path based on how the software is packaged.	*/
		if (getenv("APPDIR")) {
			/*	Extract it as if it is installed.	*/
			return fmt::format("{}/usr/lib/asset/{}", getenv("APPDIR"), relativePath);
		}
		return fmt::format("{}/asset/{}", ".", relativePath);
	}
};