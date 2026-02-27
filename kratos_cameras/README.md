# kratos_cameras

The `kratos_cameras` package provides a robust, hardware-accelerated camera streaming system for the Kratos Rover. It leverages **NVIDIA Jetson**'s hardware encoders and **GStreamer** to provide high-performance MJPEG-to-AV1 transcoding and TCP-based streaming with minimal latency and CPU overhead.

## Technical Architecture

The package is designed to handle multiple camera streams simultaneously, ensuring efficient resource utilization on Jetson platforms.

### Core Components
- **`camera_stream_node`**: The main ROS 2 node responsible for device discovery, service handling, and orchestration of camera streams.
- **`GstStreamManager`**: A C++ class that manages the lifecycle of GStreamer pipelines. It builds hardware-accelerated pipelines and monitors stream health.

### High-Performance Pipeline
The streaming system uses a custom GStreamer pipeline optimized for **NVIDIA NVMM** (Memory Management):
1. **Source**: `v4l2src` pulls MJPEG data from the camera.
2. **Decode**: `nvv4l2decoder` performs hardware MJPEG decoding.
3. **Conversion**: `nvvidconv` transforms the video into the `NV12` format required by the encoder.
4. **Encode**: `nvv4l2av1enc` encodes the stream into **AV1** using the Jetson's hardware encoder.
5. **Sink**: `tcpserversink` serves the Matroska-muxed stream over TCP on a dedicated port.

## Inner Workings

### 1. Dynamic Device Discovery
At startup and periodically (every 500ms), the node scans `/dev/` for `video*` devices.
- It verifies if a device is a real video capture device using `V4L2` ioctls.
- **Special Identification**: It inspects driver and card names to identify specific hardware like **ZED** or **GIMBAL/OBSBOT** cameras.
- **Dynamic Port Assignment**: It assigns unique TCP ports:
  - ZED: `8998`
  - GIMBAL: `8999`
  - Generic Cam1, Cam2, ...: `9000`, `9001`, etc.

### 2. Stream Profiles
The node loads predefined profiles from `camera_profiles.yaml` (e.g., `normal`, `zed`, `gimbal`). These profiles define:
- Bitrate (in kbps)
- Resolution (Width x Height)
- Framerate (FPS)
- Format (MJPG, NV12, YUY2)

## ROS 2 Interface

### Topics
| Topic | Type | Description |
| :--- | :--- | :--- |
| `/kratos/available_cameras` | `kratos_msgs/msg/CameraStreamList` | Published every 500ms. Lists all discovered cameras, their status (active/inactive), device paths, and assigned ports. |

### Services
| Service | Type | Description |
| :--- | :--- | :--- |
| `/kratos/cameras/start_stream` | `kratos_msgs/srv/StartCameraStream` | Starts a specific camera stream by name. |
| `/kratos/cameras/stop_stream` | `kratos_msgs/srv/StopCameraStream` | Stops a specific camera stream by name. |
| `/kratos/cameras/start_all` | `kratos_msgs/srv/StartAllCameras` | Attempts to start all discovered camera streams. |
| `/kratos/cameras/stop_all` | `kratos_msgs/srv/StopAllCameras` | Stops all active camera streams. |

## Configuration

The primary configuration file is `config/camera_profiles.yaml`.

```yaml
camera_stream_node:
  ros__parameters:
    stream.destination_host: 192.168.1.69
    profiles.normal.bitrate: 8000
    profiles.normal.width: 1280
    profiles.normal.height: 720
    # ... more profiles
```

## Setup & Dependencies

### System Dependencies
- `libgstreamer1.0-dev`
- `gstreamer1.0-plugins-base`
- `gstreamer1.0-plugins-good`
- `gstreamer1.0-plugins-bad` (for AV1)
- **NVIDIA L4T GStreamer plugins** (e.g., `nvv4l2decoder`, `nvv4l2av1enc`)

### Build
Standard ROS 2 build process:
```bash
colcon build --packages-select kratos_cameras
```

### Launch
To start the camera node with custom parameters:
```bash
ros2 launch kratos_cameras kratos_cameras.launch.py
```
