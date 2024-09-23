INCLUDE(FetchContent)
IF(NOT TARGET assimp)
	FetchContent_Declare(assimp_source
		GIT_REPOSITORY https://github.com/assimp/assimp.git
		GIT_TAG "v5.3.1"
	)

	FetchContent_GetProperties(assimp_source)

	IF(NOT assimp_source)
		FetchContent_Populate(assimp_source)

		OPTION(ASSIMP_BUILD_TESTS OFF)
		OPTION(ASSIMP_BUILD_ASSIMP_VIEW OFF)
		OPTION(ASSIMP_BUILD_USD_IMPORTER ON)

		ADD_SUBDIRECTORY(${assimp_source_SOURCE_DIR} ${assimp_source_BINARY_DIR} EXCLUDE_FROM_ALL)

	ELSE()
		MESSAGE(WARNING "Could not find assimp source code")
	ENDIF()
ENDIF()