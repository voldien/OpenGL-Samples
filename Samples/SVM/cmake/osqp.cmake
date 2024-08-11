INCLUDE(FetchContent)
FetchContent_Declare(osqp_source
    GIT_REPOSITORY https://github.com/osqp/osqp.git
    GIT_TAG "v0.6.3"
)

FetchContent_GetProperties(osqp_source)

IF(NOT osqp_source)
    FetchContent_Populate(osqp_source)

    ADD_SUBDIRECTORY(${osqp_source_SOURCE_DIR} ${osqp_source_BINARY_DIR} EXCLUDE_FROM_ALL)

    SET(DFLOAT ON)
    SET(PRINTING OFF)
	SET(PROFILING OFF)
	SET(CTRLC OFF)

    SET(PYTHON OFF)
    SET(MATLAB OFF)
    SET(R_LANG OFF)
    
ENDIF()
