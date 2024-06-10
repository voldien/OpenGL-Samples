FILE(GLOB SOURCE_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_OPENGL_COMMON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" )
INCLUDE(assimp)
INCLUDE(cxxopts)

ADD_LIBRARY(glCommon SHARED ${SOURCE_OPENGL_COMMON_FILES} ${HEADER_OPENGL_COMMON_FILES})
TARGET_LINK_LIBRARIES(glCommon PUBLIC fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl fragcore-imageloader glew ${OPENGL_LIBRARIES} ${SDL2_LINK_LIBRARIES})

# OpenGL dependies headers.
TARGET_INCLUDE_DIRECTORIES(glCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
ADD_DEPENDENCIES(glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler fragcore-geometry-util fragcore-window-sdl cxxopts fragcore-imageloader glew)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Importer)

INSTALL(TARGETS glCommon DESTINATION lib)