cmake_minimum_required(VERSION 3.8)
project(kratos_gazebo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(gazebo_ros REQUIRED)

# Install launch files
install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

# Install worlds
install(DIRECTORY
  worlds
  DESTINATION share/${PROJECT_NAME}/
)

# Install URDF files (Gazebo-specific)
install(DIRECTORY
  urdf
  DESTINATION share/${PROJECT_NAME}/
)
# Install Python scripts as executables
install(PROGRAMS
  scripts/fake_mavros.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME fake_mavros
)

install(DIRECTORY
  models
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
