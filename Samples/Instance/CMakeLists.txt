FILE(GLOB SOURCE_INSTANCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(Instance ${SOURCE_INSTANCE_FILES} ${HEADER_FILES})
TARGET_LINK_LIBRARIES(Instance glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(Instance glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(Instance PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS Instance DESTINATION bin)