# CMake entry point
cmake_minimum_required (VERSION 2.6)
project (agh-rcast)

find_package(OpenGL REQUIRED)
find_package(Qt4 REQUIRED)

# Compile external dependencies 
add_subdirectory (external)

# On Visual 2005 and above, this module can set the debug working directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-1c73e35")
include(CreateLaunchers)
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
	add_subdirectory(distrib)
endif(INCLUDE_DISTRIB)

list( APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")


include_directories(
	external/AntTweakBar-1.15/include/
	external/glfw-2.7.6/include/
	external/glm-0.9.4.0/
	external/glew-1.9.0/include/
	/opt/nvidia-cg-toolkit/include
	.
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

qt4_wrap_cpp(MOC_GENERATED ${MOC})
qt4_wrap_ui(UI_GENERATED ${UI})
qt4_add_resources(RCC_GENERATED ${RCC})


source_group("Form Files" FILES ${UI} )
source_group("Resource Files" FILES ${RCC} )
source_group("Generated Files" FILES ${MOC_GENERATED}   ${UI_GENERATED}  ${RCC_GENERATED} )

 



set(ALL_LIBS
	${OPENGL_LIBRARY}
	GLFW_276
	GLEW_190
	glut
	Cg
	CgGL
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

# raycast
add_executable(raycast
	raycast/raycast.cpp
	common/perlin.cpp
	common/perlin.hpp
	common/shader.cpp
	common/shader.hpp
	common/texture.cpp
	common/texture.hpp
	common/objloader.cpp
	common/objloader.hpp
)
target_link_libraries(raycast
	${ALL_LIBS}
)
# Xcode and Visual working directories
set_target_properties(raycast PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/raycast/")
create_target_launcher(raycast WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/raycast/")

SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*" )


if (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )
add_custom_command(
   TARGET raycast POST_BUILD

   COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/raycast/shader.cg" "${CMAKE_CURRENT_BINARY_DIR}"

)


elseif (${CMAKE_GENERATOR} MATCHES "Xcode" )

endif (NOT ${CMAKE_GENERATOR} MATCHES "Xcode" )

