FILE(GLOB VARIANCE_SHADOW_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB VARIANCE_SHADOW_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(VarianceShadow ${VARIANCE_SHADOW_SOURCE_FILES} ${VARIANCE_SHADOW_HEADER_FILES})
TARGET_LINK_LIBRARIES(VarianceShadow glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(VarianceShadow glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(VarianceShadow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS VarianceShadow DESTINATION bin)