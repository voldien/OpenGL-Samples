FILE(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_LIBRARY(glCommon SHARED ${SOURCE_FILES} ${HEADER_FILES} )
TARGET_LINK_LIBRARIES(glCommon PUBLIC fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler cxxopts fragcore-geometry-util fragcore-window-sdl ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LINK_LIBRARIES} )

# OpenGL dependies headers.
TARGET_INCLUDE_DIRECTORIES(glCommon PRIVATE ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
ADD_DEPENDENCIES(glCommon fragcore nekomimi-window fragcore-rgl fragcore-shader-compiler fragcore-geometry-util fragcore-window-sdl cxxopts)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Importer)
