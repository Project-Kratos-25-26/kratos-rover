# Kratos Rover ROS 2 Workspace

This repository contains the ROS 2 packages for the **Kratos Rover**, a 6-wheeled mobile robot equipped with a ZED 2i stereo camera. The workspace includes packages for robot description, Gazebo simulation, Navigation2 (Nav2) configuration, and RTAB-Map SLAM.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Packages Overview](#packages-overview)
  - [kratos_description](#kratos_description)
  - [kratos_gazebo](#kratos_gazebo)
  - [kratos_nav2](#kratos_nav2)
  - [kratos_rtabmap](#kratos_rtabmap)
- [Usage](#usage)
  - [Simulation](#simulation)
  - [Navigation](#navigation)
  - [SLAM (RTAB-Map)](#slam-rtab-map)

## Prerequisites

- **ROS 2** (Humble/Iron recommended)
- **Gazebo** (Classic)
- **Nav2** packages
- **RTAB-Map** packages
- **ZED ROS 2 Wrapper** (for camera description)

## Installation

1. **Clone the repository** into your ROS 2 workspace `src` directory:
   ```bash
   cd ~/ros2_ws/src
   git clone <repository_url> kratos-rover
   ```

2. **Install dependencies**:
   ```bash
   cd ~/ros2_ws
   rosdep install --from-paths src --ignore-src -r -y
   ```

3. **Build the workspace**:
   ```bash
   colcon build --symlink-install
   ```

4. **Source the setup script**:
   ```bash
   source install/setup.bash
   ```

## Packages Overview

### kratos_description
Contains the URDF and Xacro files defining the physical structure of the Kratos rover.
- **Robot Model**: 6-wheeled rover with a main body.
- **Sensors**: ZED 2i Stereo Camera mounted on the front.
- **Key Files**:
  - `urdf/kratos.xacro`: Main robot description file.
  - `urdf/kratos_body.xacro`: Chassis definition.
  - `urdf/kratos_left_wheel.xacro` & `urdf/kratos_right_wheel.xacro`: Wheel macros.

### kratos_gazebo
Provides the Gazebo simulation environment for the rover.
- **Launch File**: `kratos_gazebo.launch.py`
- **Features**:
  - Spawns the robot in a custom world (`cones.world`).
  - Publishes robot state using `robot_state_publisher`.
  - Includes a `fake_mavros` node for compass simulation.
  - Configures Gazebo model and resource paths.

### kratos_nav2
Configuration and launch files for the ROS 2 Navigation Stack (Nav2).
- **Launch File**: `kratos_nav2.launch.py`
- **Config**: `config/nav2_params.yaml` (Nav2 parameters).
- **Features**:
  - Launches standard Nav2 stack (`nav2_bringup`).
  - Configured for simulation time (`use_sim_time: true`).
  - SLAM is disabled by default in the launch file (can be used with `kratos_rtabmap`).

### kratos_rtabmap
Implements SLAM (Simultaneous Localization and Mapping) using RTAB-Map.
- **Launch File**: `kratos_rtabmap.launch.py`
- **Features**:
  - **RGB-D Sync**: Synchronizes RGB and Depth images from the ZED camera.
  - **Odometry**: Supports switching between Wheel Odometry (default) and Visual Odometry via the `visual_odometry` launch argument.
  - **Mapping**: Generates a 3D map and 2D occupancy grid.
  - **Visualization**: Launches `rtabmap_viz` for real-time monitoring.
- **Subscribed Topics**:
  - RGB: `/zed/zed_node/left/image_rect_color`
  - Depth: `/zed/zed_node/depth/depth_registered`
  - Camera Info: `/zed/zed_node/left/camera_info`
  - IMU: `/zed/zed_node/imu/data`
  - Odom: `/odom` (if visual odometry is disabled)

## Usage

### Simulation
To launch the robot in the Gazebo simulation environment:
```bash
ros2 launch kratos_gazebo kratos_gazebo.launch.py
```
This will open Gazebo with the Kratos rover spawned in the `cones.world`.

### Navigation
To start the Navigation stack (ensure simulation is running first):
```bash
ros2 launch kratos_nav2 kratos_nav2.launch.py
```
This will launch the Nav2 stack with the configured parameters. You can use RViz to set initial poses and navigation goals.

### SLAM (RTAB-Map)
To start mapping using RTAB-Map:

**Option 1: Using Wheel Odometry (Recommended for Sim)**
```bash
ros2 launch kratos_rtabmap kratos_rtabmap.launch.py visual_odometry:=false
```

**Option 2: Using Visual Odometry**
```bash
ros2 launch kratos_rtabmap kratos_rtabmap.launch.py visual_odometry:=true
```

This will open the RTAB-Map visualization window where you can see the map being built in real-time.
