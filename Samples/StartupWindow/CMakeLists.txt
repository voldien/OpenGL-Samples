FILE(GLOB SOURCE_STARTUP_WINDOW_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
FILE(GLOB HEADER_STARTUP_WINDOW_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

ADD_EXECUTABLE(StartupWindow ${SOURCE_STARTUP_WINDOW_FILES} ${HEADER_STARTUP_WINDOW_FILES})
TARGET_LINK_LIBRARIES(StartupWindow ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SDL2_LINK_LIBRARIES} glCommon)

TARGET_INCLUDE_DIRECTORIES(StartupWindow PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
ADD_DEPENDENCIES(StartupWindow glCommon)

INSTALL(TARGETS StartupWindow DESTINATION bin)