cmake_minimum_required(VERSION 3.16)
project(delivery_robot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# 설치된 JSON 라이브러리를 찾고, 없으면 공식 배포본을 내려받는다.
find_package(nlohmann_json 3.12.0 QUIET)
if(NOT nlohmann_json_FOUND)
  include(FetchContent)
  set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
  set(JSON_Install OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(robot_node src/robot_node.cpp)
ament_target_dependencies(robot_node rclcpp std_msgs)
target_compile_options(robot_node PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(robot_node Boost::system nlohmann_json::nlohmann_json)

install(TARGETS robot_node DESTINATION lib/${PROJECT_NAME})
ament_package()
