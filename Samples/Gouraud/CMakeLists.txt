FILE(GLOB GOURAUD_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(Gouraud ${GOURAUD_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(Gouraud glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(Gouraud glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(Gouraud PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS Gouraud DESTINATION bin)