cmake_minimum_required(VERSION 3.8)
project(kratos_nav2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

install(DIRECTORY
  launch
  config
  maps
  behavior_trees
  DESTINATION share/${PROJECT_NAME}
)

ament_package()

