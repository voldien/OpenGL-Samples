FILE(GLOB SOURCE_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
									 ${CMAKE_CURRENT_SOURCE_DIR}/Util/*.cpp)
FILE(GLOB HEADER_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h
									 ${CMAKE_CURRENT_SOURCE_DIR}/Util/*.h)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
INCLUDE(assimp)
INCLUDE(libcxxopts)

ADD_LIBRARY(glCommon SHARED ${SOURCE_OPENGL_COMMON_FILES} ${HEADER_OPENGL_COMMON_FILES})
TARGET_LINK_LIBRARIES(glCommon PUBLIC fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl fragcore-imageloader glew fragcore-scheduler-tasksch ${OPENGL_LIBRARIES} ${SDL2_LINK_LIBRARIES})

IF(JEMALLOC_LIBRARIES)
	TARGET_LINK_LIBRARIES(glCommon PUBLIC ${JEMALLOC_LIBRARIES})
ENDIF()

# OpenGL dependies headers.
TARGET_INCLUDE_DIRECTORIES(glCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/extern )
ADD_DEPENDENCIES(glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler fragcore-geometry-util fragcore-window-sdl cxxopts fragcore-imageloader fragcore-scheduler-tasksch glew)

TARGET_COMPILE_DEFINITIONS(glCommon PRIVATE FLYTHROUGH_CAMERA_IMPLEMENTATION=1)

IF(OpenMP_FOUND AND OpenMP_CXX_FOUND)
	TARGET_LINK_LIBRARIES(glCommon PRIVATE m pthread PUBLIC OpenMP::OpenMP_CXX)
	TARGET_COMPILE_OPTIONS(glCommon PUBLIC ${OpenMP_CXX_FLAGS})
ENDIF()

# 
IF((CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX) AND NOT CMAKE_COMPILER_IS_MINGW)
	TARGET_COMPILE_OPTIONS(glCommon PUBLIC "-pedantic")
	TARGET_COMPILE_OPTIONS(glCommon PUBLIC "-Wall")
ENDIF()

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Importer)

INSTALL(TARGETS glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl fragcore-imageloader glew fragcore-scheduler-tasksch DESTINATION lib)
