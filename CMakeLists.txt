cmake_minimum_required(VERSION 3.10)

# set the project name
project(frankfurtDA)

add_executable(main src/main.cpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_include_directories(main PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(main PRIVATE imgui::imgui)

find_package(EnTT CONFIG REQUIRED)
target_link_libraries(main PRIVATE EnTT::EnTT)
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE glfw)
find_package(glm CONFIG REQUIRED)
target_link_libraries(main PRIVATE glm::glm)
find_package(GLEW REQUIRED)
target_link_libraries(main PRIVATE GLEW::GLEW)
