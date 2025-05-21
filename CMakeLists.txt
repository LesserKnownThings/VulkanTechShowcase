cmake_minimum_required(VERSION 3.20)
project(VulkanTechShowcase)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLOBAL_SOURCES "")
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/Source/*.cpp")
list(APPEND GLOBAL_SOURCES ${SOURCES})

#Look for third party libs
add_compile_definitions(VK_NO_PROTOTYPES)

find_package(sdl3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(volk CONFIG REQUIRED)
##########################

add_executable(VulkanTechShowcase ${GLOBAL_SOURCES})

#Link third party libs
target_link_libraries(VulkanTechShowcase
    PRIVATE
        SDL3::SDL3
        assimp::assimp
        Vulkan::Vulkan
        volk::volk
)

find_path(VULKAN_MEMORY_ALLOCATOR_INCLUDE_DIRS "vma/vk_mem_alloc.h")
target_include_directories(VulkanTechShowcase PRIVATE ${VULKAN_MEMORY_ALLOCATOR_INCLUDE_DIRS})
######################

target_include_directories(VulkanTechShowcase PRIVATE
    ${CMAKE_SOURCE_DIR}/Source/Engine
)

set(GLOBAL_HEADERS "")
file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/Source/*.h")
list(APPEND GLOBAL_HEADERS ${HEADERS})

target_include_directories(VulkanTechShowcase PUBLIC ${GLOBAL_HEADERS})

#Copy all the Data to the build output

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" SOURCE_DIR_NATIVE)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}" BINARY_DIR_NATIVE)

add_custom_target(
    copy_files_to_output
    ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Running pre-build scripts"
    COMMAND ${CMAKE_SOURCE_DIR}/BuildScripts/CopyDataToOutput.bat ${SOURCE_DIR_NATIVE} ${BINARY_DIR_NATIVE}
)

add_dependencies(VulkanTechShowcase copy_files_to_output)