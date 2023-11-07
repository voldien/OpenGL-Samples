FILE(GLOB GRASS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(StylizedLandscape ${GRASS_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(StylizedLandscape glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(StylizedLandscape glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(StylizedLandscape PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS StylizedLandscape DESTINATION bin)