FILE(GLOB SOURCE_TERRAIN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_TERRAIN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# Simple Terrain
ADD_EXECUTABLE(SimpleTerrain ${SOURCE_TERRAIN_FILES} ${HEADER_TERRAIN_FILES})
TARGET_LINK_LIBRARIES(SimpleTerrain glCommon gl-sample-common-asset-importer)

# OpenGL dependies headers.
ADD_DEPENDENCIES(SimpleTerrain glCommon gl-sample-common-asset-importer)

INSTALL(TARGETS SimpleTerrain DESTINATION bin)