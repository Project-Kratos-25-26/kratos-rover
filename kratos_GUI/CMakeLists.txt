cmake_minimum_required(VERSION 3.16)

project(gui_vibecoded VERSION 0.1 LANGUAGES CXX)

# ──────────────────────────────────────────────
# 1. Qt auto-generation (moc, uic, rcc)
# ──────────────────────────────────────────────
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────
# 2. Find packages
# ──────────────────────────────────────────────
# ROS 2
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(kratos_msgs REQUIRED)

# Qt 6
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# GStreamer
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0)

# ──────────────────────────────────────────────
# 3. Source files
# ──────────────────────────────────────────────
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/ros_worker.cpp
    src/ros_worker.h
    src/video_worker.cpp
    src/video_worker.h
)

# ──────────────────────────────────────────────
# 4. Build the executable
# ──────────────────────────────────────────────
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(gui_vibecoded
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(gui_vibecoded ${PROJECT_SOURCES})
endif()

# ──────────────────────────────────────────────
# 5. Link libraries
# ──────────────────────────────────────────────
target_include_directories(gui_vibecoded PUBLIC ${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(gui_vibecoded 
    Qt${QT_VERSION_MAJOR}::Widgets
    ${GSTREAMER_LIBRARIES}
)

ament_target_dependencies(gui_vibecoded
    rclcpp
    std_msgs
    kratos_msgs
)

# ──────────────────────────────────────────────
# 6. Install (for `ros2 run`)
# ──────────────────────────────────────────────
install(TARGETS gui_vibecoded
    DESTINATION lib/${PROJECT_NAME}
)

# ──────────────────────────────────────────────
# 7. Finalize
# ──────────────────────────────────────────────
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(gui_vibecoded)
endif()

# MUST be the last line
ament_package()
