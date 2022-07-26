cmake_minimum_required(VERSION 3.10)

# define macros
file (STRINGS "platform.txt" _PLATFORM)
if(${_PLATFORM} STREQUAL "WSL")
	add_definitions(-DWSL)
elseif(${_PLATFORM} STREQUAL "NATIVE_UNIX")
	add_definitions(-DNATIVE_UNIX)
elseif(${_PLATFORM} STREQUAL "WINDOWS")
	add_definitions(-DWINDOWS)
endif()

# set the project name
project(Routing)

# bring headers into project
include_directories(include)
include_directories("RoutingKit/include")

# set sources
file(GLOB SOURCES "src/*.cpp")

# add the executable
add_executable(Routing ${SOURCES})

# link library needed for the vertex parsing
target_link_libraries(Routing routingkit)

target_link_directories(Routing PUBLIC "RoutingKit/lib")

# specify c++ standard
set_property(TARGET Routing PROPERTY CXX_STANDARD 17)