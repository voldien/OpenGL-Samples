FILE(GLOB VECTOR_FIELD2D_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB VECTOR_FIELD2D_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(VectorField2D ${VECTOR_FIELD2D_FILES} ${VECTOR_FIELD2D_HEADER_FILES})
TARGET_LINK_LIBRARIES(VectorField2D glCommon gl-sample-common-asset-importer)
ADD_DEPENDENCIES(VectorField2D glCommon gl-sample-common-asset-importer)

TARGET_INCLUDE_DIRECTORIES(VectorField2D PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})