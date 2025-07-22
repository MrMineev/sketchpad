cmake_minimum_required(VERSION 3.15)
project(main CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML include and library paths (Homebrew on Apple Silicon)
set(SFML_INCLUDE_DIR "/opt/homebrew/Cellar/sfml/2.6.1/include")
set(SFML_LIBRARY_DIR "/opt/homebrew/Cellar/sfml/2.6.1/lib")

# Add include directories
include_directories(${SFML_INCLUDE_DIR})

# Gather all .cpp files under source tree, excluding the build directory
file(GLOB_RECURSE ALL_CPP_REL
    RELATIVE ${CMAKE_SOURCE_DIR}
    "*.cpp"
)

# Filter out any files in the build directory
set(SRC_FILES "")
foreach(src IN LISTS ALL_CPP_REL)
    if(NOT src MATCHES "^build/.*")
        list(APPEND SRC_FILES "${CMAKE_SOURCE_DIR}/${src}")
    endif()
endforeach()

# Define the executable target
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Link SFML libraries and OpenGL framework
target_link_directories(${PROJECT_NAME} PRIVATE ${SFML_LIBRARY_DIR})
target_link_libraries(${PROJECT_NAME}
    sfml-graphics
    sfml-window
    sfml-system
    sfml-network
    "-framework OpenGL"
)

# Enable the clean target if needed (CMake auto-cleans build/ dir)
# add_custom_target(clean-all
#     COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
# )

