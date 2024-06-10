INCLUDE(FetchContent)

IF(NOT TARGET cxxopts)
	FetchContent_Declare(cxxopts_source
		GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
		GIT_TAG v3.2.1
	) # or whatever tag you want

	FetchContent_GetProperties(cxxopts_source)

	IF(NOT cxxopts_source_POPULATED)
		FetchContent_Populate(cxxopts_source)
	ELSE()
		MESSAGE( WARNING "Could not find cxxopts source code")
	ENDIF()

	IF(cxxopts_source_POPULATED)
		ADD_SUBDIRECTORY(${cxxopts_source_SOURCE_DIR} ${cxxopts_source_BINARY_DIR} EXCLUDE_FROM_ALL)
	ENDIF()
ENDIF()