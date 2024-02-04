FILE(GLOB SOURCE_BSP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_BSP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(BinarySpacePartitioning ${SOURCE_BSP_FILES} ${HEADER_BSP_FILES})
TARGET_LINK_LIBRARIES(BinarySpacePartitioning glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(BinarySpacePartitioning glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(BinarySpacePartitioning PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS BinarySpacePartitioning DESTINATION bin)