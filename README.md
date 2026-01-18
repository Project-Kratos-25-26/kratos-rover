# Kratos Rover ROS 2 Workspace

This repository contains the ROS 2 packages for the **Kratos Rover**, a 6-wheeled autonomous mobile robot equipped with a ZED 2i stereo camera. The workspace provides a complete stack for robot simulation, autonomous navigation, simultaneous localization and mapping (SLAM), and real-world deployment.

## Table of Contents
- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Packages Overview](#packages-overview)
  - [kratos_description](#kratos_description)
  - [kratos_bringup](#kratos_bringup)
  - [kratos_gazebo](#kratos_gazebo)
  - [kratos_nav2](#kratos_nav2)
  - [kratos_rtabmap](#kratos_rtabmap)
  - [kratos_slam_toolbox](#kratos_slam_toolbox)
  - [kratos_localization](#kratos_localization)
  - [kratos_vel_clamp](#kratos_vel_clamp)
- [Usage](#usage)
  - [Simulation](#simulation)
  - [Navigation](#navigation)
  - [SLAM Options](#slam-options)
  - [System Integration](#system-integration)
- [Real Robot Deployment](#real-robot-deployment)

## Overview

The Kratos Rover is a fully autonomous rover designed for outdoor navigation and mapping. The software stack includes:

- **Robot Model**: 6-wheeled differential drive rover with integrated ZED 2i stereo camera
- **Simulation**: Gazebo simulation environment with multiple world configurations
- **Navigation**: ROS 2 Navigation Stack (Nav2) for autonomous goal-based navigation
- **SLAM**: Dual SLAM implementations (RTAB-Map and Slam Toolbox)
- **Localization**: GPS-free localization using visual odometry or wheel encoders
- **Safety**: Velocity clamping for smooth motion control

## Prerequisites

- **ROS 2** (Humble or Iron recommended)
- **Gazebo** (gazebo_ros package)
- **Nav2** packages (`nav2_bringup`, `nav2_core`)
- **RTAB-Map** packages (`rtabmap`, `rtabmap_ros`, `rtabmap_sync`)
- **Slam Toolbox** package
- **ZED ROS 2 Wrapper** (for real camera hardware)
- **Python 3.8+**

### Optional Dependencies
- **YOLOv5** (for cone detection - used by sibling packages)
- **OpenCV** (for image processing)

## Installation

1. **Clone the repository** into your ROS 2 workspace `src` directory:
   ```bash
   cd ~/ros2_ws/src
   git clone https://github.com/Project-Kratos-25-26/kratos-rover.git kratos-rover
   ```

2. **Install system dependencies**:
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
Contains the URDF and Xacro files defining the physical structure and sensors of the Kratos rover.

**Contents**:
- `urdf/kratos.xacro`: Main robot description with all components
- `urdf/kratos_body.xacro`: Chassis and body structure
- `urdf/kratos_left_wheel.xacro`: Left wheel macro definition
- `urdf/kratos_right_wheel.xacro`: Right wheel macro definition
- `meshes/`: 3D models for visualization

**Robot Features**:
- **Dimensions**: 6-wheeled differential drive platform
- **Sensors**: 
  - ZED 2i stereo camera (front-mounted)
  - Integrated IMU
  - Wheel encoders (for odometry)
- **Base Frame**: `base_link` (robot center)
- **Camera Frame**: `zed_camera_link`

**Launch**:
```bash
ros2 launch kratos_bringup robot_state_publisher.launch.py
```

### kratos_bringup
Entry point package for launching the robot platform components.

**Launch Files**:
- `launch/bringup.launch.py`: Launches `robot_state_publisher` with the URDF

**Purpose**: Provides a simple way to load the robot description and publish transforms for TF tree.

### kratos_gazebo
Provides Gazebo simulation environment with physics, sensor simulation, and visual rendering.

**Launch File**: `kratos_gazebo.launch.py`

**Features**:
- Spawns Kratos rover in Gazebo with physics enabled
- Publishes robot state using `robot_state_publisher`
- Configures Gazebo model paths and resource paths
- Launches `robot_state_publisher` for TF broadcasting
- Includes `fake_mavros` node for compass simulation (magnetic heading)

**World Files Available**:
- `worlds/test.world`: Basic flat test world
- `worlds/cones.world`: World with cone obstacles for navigation testing
- `worlds/cones1.world`: Alternative cone configuration
- `worlds/empty.world`: Minimal environment

**Launch**:
```bash
# Default (test.world)
ros2 launch kratos_gazebo kratos_gazebo.launch.py

# With custom world
ros2 launch kratos_gazebo kratos_gazebo.launch.py world:=cones.world
```

**Default Topics Published**:
- `/robot_state`: Robot state updates
- `/gazebo/model_states`: Gazebo model positions
- `/tf`: Transform tree
- `/tf_static`: Static transforms

### kratos_nav2
Configuration and launch orchestration for the ROS 2 Navigation Stack (Nav2).

**Launch File**: `kratos_nav2.launch.py`

**Configuration Files**:
- `config/nav2_params.yaml`: Navigation parameters (controller, planner, recoveries)

**Features**:
- Launches standard Nav2 stack with custom configurations
- Configured for simulation time (`use_sim_time: true`)
- SLAM disabled by default (can be run separately with RTAB-Map)
- Includes planning, controller, and recovery behaviors

**Navigation Plugins**:
- **Planner**: DWB Local Planner or equivalent
- **Global Planner**: Navfn
- **Recovery Behaviors**: Spin, Back-up, Clear costmap

**Launch**:
```bash
# Start Nav2 stack (ensure Gazebo is running)
ros2 launch kratos_nav2 kratos_nav2.launch.py
```

**Usage with RViz**:
```bash
# In a separate terminal, launch RViz
rviz2 -d <path_to_rviz_config>
```

Then set initial pose and navigation goals in RViz.

### kratos_rtabmap
Implements SLAM using RTAB-Map (Real-Time Appearance-Based Mapping).

**Launch Files**:
- `kratos_rtabmap.launch.py`: Full RTAB-Map stack with ZED camera integration
- `kratos_rtabmap_gazebo.launch.py`: Gazebo-optimized RTAB-Map configuration

**Key Features**:

**RGB-D Synchronization**:
- Synchronizes RGB and Depth images from ZED camera
- Critical sync parameters configured:
  - `approx_sync_max_interval: 1.0` (message sync tolerance)
  - `queue_size: 20` (buffer size)
  - `sync_queue_size: 20` (RGB-D sync buffer)

**Odometry Options**:
- **Wheel Odometry** (default): Uses `/odom` topic from wheel encoders
- **Visual Odometry**: Computes odometry from camera movement (set `visual_odometry:=true`)

**Output**:
- Generates 3D point cloud map
- Creates 2D occupancy grid
- Publishes transforms between `odom`, `map`, and `base_link`

**Subscribed Topics**:
```
- rgb/image → /zed/zed_node/left/image_rect_color
- rgb/camera_info → /zed/zed_node/left/camera_info
- depth/image → /zed/zed_node/depth/depth_registered
- imu → /zed/zed_node/imu/data
- odom → /odom (if visual odometry disabled)
```

**Published Topics**:
- `/rtabmap/map`: 3D point cloud
- `/grid_map`: 2D occupancy grid
- `/rtabmap_ros/info`: Mapping statistics
- `/tf`: Transform tree updates

**Launch**:
```bash
# Using wheel odometry (recommended for sim)
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py visual_odometry:=false

# Using visual odometry
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py visual_odometry:=true

# Reset database and start fresh
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py new_map:=true
```

**Visualization**:
- RTAB-Map includes built-in visualization window showing:
  - 3D map in real-time
  - Loop closures
  - Odometry trajectory
  - Feature matching

### kratos_slam_toolbox
Alternative SLAM implementation using Slam Toolbox (based on Google Cartographer).

**Launch File**: `kratos_slam_launch.py`

**Features**:
- Online asynchronous SLAM operation
- Lower CPU overhead compared to RTAB-Map
- Map loading/saving capabilities
- Optimized for real-time operation

**Configuration**:
- `config/mapper_params_online_async.yaml`: Slam Toolbox parameters

**Auto-Load Feature**:
- Automatically loads saved maps on startup via `load_map_on_start.py`
- Map directory: `maps/auto_map`

**Launch**:
```bash
ros2 launch kratos_slam_toolbox kratos_slam_launch.py
```

**Output**:
- `/map`: Occupancy grid
- `/tf`: Map-to-odom transform
- `/map_metadata`: Map metadata

### kratos_localization
Package for localization configuration and parameters (currently configuration only).

**Contents**:
- `config/`: Localization parameter files

**Purpose**: Provides a dedicated namespace for localization-specific parameters, allowing separation of concerns between mapping and localization tasks.

### kratos_vel_clamp
Velocity safety and smoothing module for command filtering.

**Scripts**:

1. **`velclamp.py`**: Angular velocity clamping node
   - **Function**: Limits angular velocity to safe operating range
   - **Parameters**:
     - `min_w`: Minimum angular velocity (0.8 rad/s)
     - `max_w`: Maximum angular velocity (1.5 rad/s)
     - `deadband`: Noise threshold around zero (0.05 rad/s)
   - **Subscribed Topic**: `/cmd_vel` (Twist)
   - **Published Topic**: `/cmd_vel_filt` (filtered Twist)

2. **`publish_empty_map.py`**: Map placeholder publisher
   - **Function**: Publishes an empty occupancy grid as placeholder
   - **Published Topic**: `/map` (OccupancyGrid)
   - **Use Case**: Provides map when SLAM is not running

3. **`littlekids.py`**: Additional velocity control (function TBD)

**Installation**:
```bash
chmod +x scripts/*.py
```

## Usage

### Simulation

**Step 1: Start Gazebo simulation**
```bash
ros2 launch kratos_gazebo kratos_gazebo.launch.py
```

This launches:
- Gazebo with the Kratos rover
- Robot state publisher (TF tree)
- Fake compass (magnetic heading)

**Step 2 (Optional): Launch Nav2 for autonomous navigation**
```bash
# In a new terminal
ros2 launch kratos_nav2 kratos_nav2.launch.py
```

**Step 3 (Optional): Launch SLAM for mapping**
```bash
# Option A: Using wheel odometry (recommended for sim)
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py visual_odometry:=false

# Option B: Using visual odometry
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py visual_odometry:=true
```

**Step 4: Visualize with RViz**
```bash
rviz2
```

Configure RViz to display:
- `/tf` (transforms)
- `/map` (occupancy grid if SLAM is running)
- `/rtabmap/map` (3D point cloud if RTAB-Map running)
- Robot model
- Navigation goals

### Navigation

To autonomously navigate the robot using Nav2:

1. **Ensure Gazebo and Nav2 are running** (see Simulation section)

2. **In RViz**:
   - Set **Initial Pose**: Use "2D Pose Estimate" tool to place robot on known map location
   - Set **Nav Goal**: Use "Nav2 Goal" tool to send robot to target

3. **Monitor Navigation**:
   ```bash
   ros2 topic echo /robot_pose
   ros2 topic echo /plan
   ```

### SLAM Options

**Option 1: RTAB-Map (Recommended for 3D mapping)**
```bash
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py
```
- Produces 3D point cloud and 2D occupancy grid
- Better loop closure detection
- Higher computational cost
- Built-in visualization window

**Option 2: Slam Toolbox (Recommended for real-time on resource-limited systems)**
```bash
ros2 launch kratos_slam_toolbox kratos_slam_launch.py
```
- Real-time async SLAM
- Lower CPU overhead
- Map persistence (auto-loads on startup)
- Suitable for onboard computation

**Save SLAM Map**:
```bash
# RTAB-Map database
ros2 bag record /rtabmap/map -o kratos_map

# Slam Toolbox map
ros2 service call /save_map std_srvs/Trigger
```

### System Integration

**Full Stack Launch (Simulation + Nav2 + SLAM)**:
```bash
# Terminal 1: Gazebo
ros2 launch kratos_gazebo kratos_gazebo.launch.py

# Terminal 2: SLAM (choose one)
ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py  # or
ros2 launch kratos_slam_toolbox kratos_slam_launch.py

# Terminal 3: Nav2
ros2 launch kratos_nav2 kratos_nav2.launch.py

# Terminal 4: RViz
rviz2
```

**Alternative: Single launch file approach**
```bash
ros2 launch kratos_bringup bringup.launch.py
```

**Velocity Control**:
The `kratos_vel_clamp` package provides command filtering:
```bash
# Node that clamps angular velocity
ros2 run kratos_vel_clamp velclamp.py

# Subscribe to: /cmd_vel
# Publish to: /cmd_vel_filt
```

## Real Robot Deployment

To deploy this stack on the physical Kratos Rover with a real ZED 2i camera:

### 1. General Configuration

**Disable Simulation Time**:
- Set `use_sim_time: false` in all launch files and configuration YAML files

**Ensure ZED Camera is Running**:
```bash
ros2 launch zed_wrapper zed.launch.py
# Verify topics are published:
ros2 topic list | grep zed
```

### 2. RTAB-Map Configuration

**File**: `kratos_rtabmap/launch/kratos_rtabmap.launch.py` or `kratos_rtabmap_gazebo.launch.py`

**Changes Required**:

```python
parameters = [{
    'use_sim_time': False,  # CRITICAL: Change from True to False
    'frame_id': 'base_link',
    'map_frame_id': 'map',
    'publish_tf': True,
    'subscribe_rgbd': True,
    'subscribe_odom': True,
    'wait_imu_to_init': True,
    # ... rest of parameters
}]
```

**Topic Remappings**:
Verify that topic names match your ZED camera configuration:
```python
common_remappings = [
    ('rgb/image', '/zed/zed_node/left/image_rect_color'),
    ('rgb/camera_info', '/zed/zed_node/left/camera_info'),
    ('depth/image', '/zed/zed_node/depth/depth_registered'),
    ('imu', '/zed/zed_node/imu/data')
]
```

If using a different ZED camera namespace (e.g., `/zed2i`), update remappings accordingly.

**Odometry Source**:
- If wheel encoders are available: `visual_odometry:=false` (default, uses `/odom`)
- If no wheel encoders: `visual_odometry:=true` (visual odometry from camera)

### 3. Nav2 Configuration

**File**: `kratos_nav2/launch/kratos_nav2.launch.py`

```python
launch_arguments={
    'use_sim_time': 'false',  # CRITICAL: Change from 'true'
    'autostart': 'true',
    'slam': 'false',
    'params_file': params_file
}.items()
```

**File**: `kratos_nav2/config/nav2_params.yaml`

Change all instances of:
```yaml
use_sim_time: true
```

To:
```yaml
use_sim_time: false
```

Also verify:
- `robot_radius`: Should match physical robot dimensions (currently 0.3m)
- `inflation_radius`: Clearance buffer around obstacles
- `max_vel_x`, `max_vel_y`, `max_omega`: Physical velocity limits

### 4. Hardware Drivers

**ZED Camera Setup**:
```bash
# Install ZED ROS 2 wrapper
sudo apt install ros-${ROS_DISTRO}-zed-wrapper

# Start ZED camera node
ros2 launch zed_wrapper zed.launch.py
```

**Wheel Odometry Setup**:
- Publish `/odom` topic with encoder-based odometry
- Provides base TF transform: `odom` → `base_link`

**IMU Integration**:
- Ensure ZED camera publishes `/zed/zed_node/imu/data` for SLAM sync

### 5. TF Tree Setup

**Static Transforms**:
Ensure the following transforms are published:
- `base_link` → `zed_camera_link`: Camera offset from robot center
- `base_link` → other sensors: Additional sensor mounts

**Create Launch File** (if needed):
```bash
ros2 launch kratos_description kratos_description.launch.py
```

Or use:
```bash
ros2 launch kratos_bringup bringup.launch.py
```

### 6. Troubleshooting

**SLAM Topics Not Syncing**:
- Check ZED camera publishing frequency (should be 30+ Hz)
- Verify depth image resolution (QVGA recommended for SLAM)
- Check network latency (Gazebo adds artificial delays)

**Map Drift**:
- Increase `wait_imu_to_init` timeout
- Adjust `approx_sync_max_interval` (higher = more tolerance)
- Use higher visual odometry quality (if available)

**Navigation Failures**:
- Verify `use_sim_time` is correctly set on ALL nodes
- Check robot footprint matches physical dimensions
- Ensure map coordinates are in `map` frame

**GPS Alternative** (if available):
- Run GPS-based localization
- Publish `/gps/fix` topic for Extended Kalman Filter

## Launch File Quick Reference

| Package | Launch File | Command | Use Case |
|---------|-------------|---------|----------|
| kratos_bringup | bringup.launch.py | `ros2 launch kratos_bringup bringup.launch.py` | Publish TF tree |
| kratos_gazebo | kratos_gazebo.launch.py | `ros2 launch kratos_gazebo kratos_gazebo.launch.py` | Start simulation |
| kratos_nav2 | kratos_nav2.launch.py | `ros2 launch kratos_nav2 kratos_nav2.launch.py` | Autonomous nav |
| kratos_rtabmap | kratos_rtabmap_gazebo.launch.py | `ros2 launch kratos_rtabmap kratos_rtabmap_gazebo.launch.py` | SLAM (simulation) |
| kratos_rtabmap | kratos_rtabmap.launch.py | `ros2 launch kratos_rtabmap kratos_rtabmap.launch.py` | SLAM (real robot) |
| kratos_slam_toolbox | kratos_slam_launch.py | `ros2 launch kratos_slam_toolbox kratos_slam_launch.py` | Cartographer SLAM |

## Common Commands

```bash
# Check all topics
ros2 topic list

# Monitor a specific topic
ros2 topic echo /topic_name

# Check TF tree
ros2 run tf2_tools view_frames

# Record a bag
ros2 bag record -a -o kratos_data

# Play back a bag
ros2 bag play kratos_data/

# Monitor nodes
rqt_graph
```

## Project Structure

```
kratos-rover/
├── kratos_bringup/           # Entry point package
│   └── launch/
│       └── bringup.launch.py
├── kratos_description/       # Robot URDF/Xacro
│   ├── urdf/
│   ├── meshes/
│   └── package.xml
├── kratos_gazebo/            # Simulation environment
│   ├── launch/
│   ├── worlds/
│   └── package.xml
├── kratos_nav2/              # Navigation configuration
│   ├── config/
│   ├── launch/
│   └── package.xml
├── kratos_rtabmap/           # RTAB-Map SLAM
│   ├── launch/
│   └── package.xml
├── kratos_slam_toolbox/      # Slam Toolbox SLAM
│   ├── config/
│   ├── launch/
│   ├── maps/
│   └── package.xml
├── kratos_localization/      # Localization config
│   ├── config/
│   └── package.xml
├── kratos_vel_clamp/         # Velocity control
│   ├── scripts/
│   └── package.xml
├── README.md                 # This file
└── .git/                     # Version control
```

## Contributing

To contribute to the Kratos Rover project:

1. Create a feature branch: `git checkout -b feature/your-feature`
2. Make your changes
3. Test thoroughly (simulation + real robot if applicable)
4. Commit with clear messages: `git commit -m "Description of changes"`
5. Push to GitHub: `git push origin feature/your-feature`
6. Create a Pull Request with detailed description

## License

This project is part of the Project Kratos initiative. Please see LICENSE file for details.

## Support & Contact

For issues, bugs, or questions:
- Create an issue on GitHub
- Contact the Project Kratos team

## Recent Updates

**January 19, 2026**:
- Updated RTAB-Map launch configuration with sync parameter fixes
- Added `new_map` launch argument for database reset control
- Comprehensive README update with full package documentation
- Added deployment guidelines for real robot usage

---

**Last Updated**: January 19, 2026  
**Repository**: https://github.com/Project-Kratos-25-26/kratos-rover

