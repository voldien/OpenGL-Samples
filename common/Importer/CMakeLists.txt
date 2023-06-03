FILE(GLOB SOURCE_IMPORTER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_IMPORTER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_LIBRARY(gl-sample-common-asset-importer ${SOURCE_IMPORTER_FILES} ${HEADER_IMPORTER_FILES})
TARGET_LINK_LIBRARIES(gl-sample-common-asset-importer PUBLIC fmt fragcore-imageloader glCommon PRIVATE assimp)
ADD_DEPENDENCIES(gl-sample-common-asset-importer glCommon fragcore-imageloader)

TARGET_INCLUDE_DIRECTORIES(gl-sample-common-asset-importer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

INSTALL(TARGETS gl-sample-common-asset-importer DESTINATION lib)