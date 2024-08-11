LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
INCLUDE(osqp)
LIST(APPEND CMAKE_MODULE_PATH "${osqp_source_BINARY_DIR}")

INCLUDE(FetchContent)
FetchContent_Declare(osqpeigen_source
    GIT_REPOSITORY https://github.com/robotology/osqp-eigen.git
    GIT_TAG "v0.8.1"
)

FetchContent_GetProperties(osqpeigen_source)

SET(osqp_DIR ${osqp_source_BINARY_DIR})
IF(NOT osqpeigen_source)
    FetchContent_Populate(osqpeigen_source)

    ADD_SUBDIRECTORY(${osqpeigen_source_SOURCE_DIR} ${osqpeigen_source_BINARY_DIR} EXCLUDE_FROM_ALL)

    ADD_DEPENDENCIES(OsqpEigen osqp)

ENDIF()
