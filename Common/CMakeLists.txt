FILE(GLOB SOURCE_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
									${CMAKE_CURRENT_SOURCE_DIR}/Scene/*.cpp
									 ${CMAKE_CURRENT_SOURCE_DIR}/Util/*.cpp
									 ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessing/*.cpp)
FILE(GLOB HEADER_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h
									 ${CMAKE_CURRENT_SOURCE_DIR}/PostProcessing/*.h
									 ${CMAKE_CURRENT_SOURCE_DIR}/Scene/*.h
									 ${CMAKE_CURRENT_SOURCE_DIR}/Util/*.h)
SOURCE_GROUP(Common FILES ${HEADER_OPENGL_COMMON_FILES} ${SOURCE_OPENGL_COMMON_FILES})

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
INCLUDE(libcxxopts)

ADD_LIBRARY(glCommon SHARED ${SOURCE_OPENGL_COMMON_FILES} ${HEADER_OPENGL_COMMON_FILES})
TARGET_LINK_LIBRARIES(glCommon PUBLIC fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl2 fragcore-imageloader glew fragcore-scheduler-tasksch ${OPENGL_LIBRARIES} ${SDL2_LINK_LIBRARIES} assimp)

IF(JEMALLOC_LIBRARIES)
	TARGET_LINK_LIBRARIES(glCommon PUBLIC ${JEMALLOC_LIBRARIES})
ENDIF()

# OpenGL dependies headers.
TARGET_INCLUDE_DIRECTORIES(glCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/extern )
ADD_DEPENDENCIES(glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler fragcore-geometry-util fragcore-window-sdl2 cxxopts fragcore-imageloader fragcore-scheduler-tasksch glew)

TARGET_COMPILE_DEFINITIONS(glCommon PRIVATE FLYTHROUGH_CAMERA_IMPLEMENTATION=1)

# Make use of modern features with GLM.
TARGET_COMPILE_DEFINITIONS(glCommon PUBLIC GLM_FORCE_CXX17 GLM_FORCE_INTRINSICS)

#
IF(OpenMP_FOUND AND OpenMP_CXX_FOUND)
	TARGET_LINK_LIBRARIES(glCommon PRIVATE m pthread PUBLIC OpenMP::OpenMP_CXX)
	TARGET_COMPILE_OPTIONS(glCommon PUBLIC ${OpenMP_CXX_FLAGS} -fopenmp-simd)
ENDIF()

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Importer)

INSTALL(TARGETS glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl2 fragcore-imageloader glew fragcore-scheduler-tasksch DESTINATION lib)
