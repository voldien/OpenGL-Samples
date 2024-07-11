FILE(GLOB RIGIDBODY_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB RIGIDBODY_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(RigidBody ${RIGIDBODY_SOURCE_FILES} ${RIGIDBODY_HEADER_FILES})
TARGET_LINK_LIBRARIES(RigidBody glCommon gl-sample-common-asset-importer fragcore-physic-bullet fragcore-physic-interface)
ADD_DEPENDENCIES(RigidBody glCommon gl-sample-common-asset-importer fragcore-physic-bullet fragcore-physic-interface)

TARGET_INCLUDE_DIRECTORIES(RigidBody PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS RigidBody DESTINATION bin)