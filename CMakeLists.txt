CMAKE_MINIMUM_REQUIRED(VERSION 3.9...3.22)
PROJECT(OpenGLSamples)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "" FORCE)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "" FORCE)

FIND_PACKAGE(PkgConfig QUIET)
FIND_PACKAGE(OpenMP QUIET)

#
IF(PKG_CONFIG_FOUND)
	MESSAGE(STATUS "Found pkg-config: ${PKG_CONFIG_EXECUTABLE} -- version ${PKG_CONFIG_VERSION_STRING}.")
	PKG_CHECK_MODULES(SDL2 QUIET sdl2)
ELSE()
	MESSAGE(ERROR "Required library not found")
ENDIF()

IF(OPENGL_FOUND)
	MESSAGE(STATUS "OpenGL found: ${OPENGL_LIBRARIES}.")
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
	MESSAGE(STATUS "Compile for release.")
	ADD_DEFINITIONS(-O2)
	SET(CMAKE_RELEASE TRUE)
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s ${OpenMP_CXX_FLAGS}")
	SET(SIMD_FLAGS "-ftree-vectorize -ftree-slp-vectorize")
ELSE()
	MESSAGE(STATUS "Compile for debug.")
	ADD_DEFINITIONS(-g3 -O0)
	SET(CMAKE_DEBUG TRUE)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} ${OpenMP_CXX_FLAGS}")

	# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	# SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	# SET(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

	# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,shift,shift-exponent,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,float-divide-by-zero,float-cast-overflow,nonnull-attribute,returns-nonnull-attribute,bool,enum,vptr,pointer-overflow,builtin -fno-sanitize-recover=all")
	# SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined,shift,shift-exponent,integer-divide-by-zero,unreachable,vla-bound,null,return,signed-integer-overflow,bounds,float-divide-by-zero,float-cast-overflow,nonnull-attribute,returns-nonnull-attribute,bool,enum,vptr,pointer-overflow,builtin -fno-sanitize-recover=all")
	# SET(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")

	# SET(BUILD_WITH_UBSAN ON)
	# SET(BUILD_WITH_ASAN ON)
ENDIF()

# ###########################################
# Backend libraries
# ###########################################
SET(MIMI_VULKAN_SUPPORT OFF)
SET(MIMI_TERMINAL_SUPPORT OFF)
SET(MIMI_OPENGL_SUPPORT ON)
SET(MIMI_WINDOW_SDL2_SUPPORT ON)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi EXCLUDE_FROM_ALL)
# Fragcore specific libraries
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/image/imageloader EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/taskscheduler/TaskSch EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/geometryutil EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/audio EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/audio/openal EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/physic EXCLUDE_FROM_ALL)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/extern/nekomimi/extern/fragcore/modules/physic/bullet EXCLUDE_FROM_ALL)

# AVX If supported
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${AVX_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${AVX_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${AVX_FLAGS}")

# SIMD
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SIMD_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${SIMD_FLAGS}")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${SIMD_FLAGS}")

# ##################################################
# Backend common logic for all OpenGL Samples
# ###################################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/common)

# ###################################
# OpenGL Samples
# ###################################
############### Finished ###############
########################################
# Basic Samples
# ###################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/StartupWindow)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Triangle)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Texture)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/AlphaClipping)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/PointLight)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/PhongBlinn)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/NormalMapping)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Skybox)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Blending)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Instance)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Normal)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MipMapVisual)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ShadowMapping)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ShadowPointLight)
# ###################################
# Advanced Samples
# ###################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Fog)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MultiPass)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/AmbientOcclusion)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SubSurfaceScattering)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VideoPlayback)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Tessellation)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Gouraud)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Rigidbody)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ProjectedShadow)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Panoramic)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SkinnedMesh)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SimpleOcean)

############## Work In Progress ####################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Deferred)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/FrustumCulling)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/FrustumCullingCompute)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ModelViewer)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/PhysicalBasedRendering)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Subgroup)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ShadowVolume)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SimpleReflection)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Ocean)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Terrain)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VariableRateShading)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/OcclusionCulling)

#ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/SVM)

#######################################
# Compute Shader Based Samples
#######################################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Mandelbrot)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ReactionDiffusion)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/ComputeGroupVisual)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/GameOfLife)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/MarchingCube)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VectorField2D)

############## Work In Progress ####################
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/VectorField)

ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/InfinateWorld)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/RayTracing)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/Sort)
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Samples/CirclePacking)

####################################
# Shader Source Files
####################################
FILE(GLOB_RECURSE GLSL_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.fs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.gs
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.geom
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tese
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tesc
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.task
	${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.mesh
)

####################################
# SPIR-V Compiler
####################################
FIND_PROGRAM(GLSLLANGVALIDATOR glslangValidator)
FIND_PROGRAM(GLSLC glslc)

FOREACH(GLSL ${GLSL_SOURCE_FILES})
	GET_FILENAME_COMPONENT(FILE_NAME ${GLSL} ABSOLUTE)


	#glslangValidator
	SET(SPIRV "${FILE_NAME}.spv")
	IF(GLSLLANGVALIDATOR)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS -g0)
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -gVS -Od)
		ENDIF()
		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/Shaders/"
			COMMAND ${GLSLLANGVALIDATOR} --define-macro gl_InstanceID=gl_InstanceIndex --define-macro gl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/Shaders/common --target-env vulkan1.1 ${SPIRV_BUILD_SETTINGS} -o ${SPIRV} ${GLSL} 
			DEPENDS ${GLSL})
	ELSEIF(GLSLC)
		IF(CMAKE_BUILD_TYPE STREQUAL "Release")
			SET(SPIRV_BUILD_SETTINGS -O)
		ELSE()
			SET(SPIRV_BUILD_SETTINGS -g -Od)
		ENDIF()

		ADD_CUSTOM_COMMAND(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "${EXECUTABLE_OUTPUT_PATH}/Shaders/"
			COMMAND ${GLSLC} -Dgl_InstanceID=gl_InstanceIndex -Dgl_VertexID=gl_VertexIndex  -I${CMAKE_CURRENT_SOURCE_DIR}/Shaders/common --target-spv=spv1.3 -x glsl ${SPIRV_BUILD_SETTINGS} -Werror --target-env=vulkan1.1 -o ${SPIRV} ${GLSL} 
			DEPENDS ${GLSL})
	ENDIF()

	LIST(APPEND SPIRV_BINARY_FILES ${SPIRV})
ENDFOREACH(GLSL)

ADD_CUSTOM_TARGET(
	Shaders ALL
	DEPENDS ${SPIRV_BINARY_FILES}
)

## External Asset
ADD_CUSTOM_TARGET(
	DownloadAsset
	DEPENDS ${CMAKE_SOURCE_DIR}/asset/downloaded
)

ADD_CUSTOM_COMMAND(
	OUTPUT ${CMAKE_SOURCE_DIR}/asset/downloaded
	COMMAND ${CMAKE_COMMAND} -E make_directory  ${CMAKE_SOURCE_DIR}/asset/zip
	COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/download_asset.py ${CMAKE_SOURCE_DIR}/asset/
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_SOURCE_DIR}/asset/downloaded
)

# All shader files.
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/Shaders ${CMAKE_CURRENT_BINARY_DIR}/Shaders SYMBOLIC)
FILE(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/asset ${CMAKE_CURRENT_BINARY_DIR}/asset SYMBOLIC)