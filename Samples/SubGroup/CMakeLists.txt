FILE(GLOB SOURCE_SUBGROUP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(SubGroupArea ${SOURCE_SUBGROUP_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(SubGroupArea glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(SubGroupArea glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(SubGroupArea PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS SubGroupArea SkyboxPanoramic DESTINATION bin)