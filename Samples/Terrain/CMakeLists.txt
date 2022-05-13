FILE(GLOB SOURCE_TERRAIN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_TERRAIN_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(Terrain  ${SOURCE_TERRAIN_FILES} ${HEADER_TERRAIN_FILES} )
TARGET_LINK_LIBRARIES(Terrain glCommon gl-sample-common-asset-importer)

# OpenGL dependies headers.
ADD_DEPENDENCIES(Terrain glCommon gl-sample-common-asset-importer)
