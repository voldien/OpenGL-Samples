FILE(GLOB SOURCE_AMBIENT_OCCLUSION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_AMBIENT_OCCLUSION_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(AmbientOcclusion ${SOURCE_AMBIENT_OCCLUSION_FILES} ${HEADER_AMBIENT_OCCLUSION_FILES})
TARGET_LINK_LIBRARIES(AmbientOcclusion glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(AmbientOcclusion glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(AmbientOcclusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS AmbientOcclusion DESTINATION bin)