# Kratos GUI Developer Guide

Welcome to the `kratos_gui` development guide. This document explains the architecture, features, how to interact with the GUI, and how to build the package.

## 1. Architecture Overview

The `kratos_gui` is a Qt6-based monitoring dashboard allowing operators to view multiple camera streams, manage layouts, and monitor joystick node outputs on the Kratos Rover.

The core architecture consists of four main components:
- `MainWindow`: The outer shell and tab manager. It controls the main window state, the top-level application hotkeys, and the instantiation of new monitoring tabs (`CameraWidget`, `JoyNodeWidget`).
- `CameraWidget`: A dedicated tab for viewing camera streams. It implements a fully dynamic `QGridLayout` allowing users to view up to 16 cameras simultaneously. It handles layout selection, sizing, and stream assignments.
- `JoyNodeWidget`: A dedicated tab for viewing raw or processed joystick data.
- `RosWorker`: The bridge between the Qt GUI thread and the ROS2 environment. It runs on a dedicated `QThread` and uses signals (`camerasUpdated`) and slots (`callStartStream`, `callStopStream`) to communicate states back and forth robustly without blocking the UI.

## 2. Key Features

### Dynamic Grid Layout
The `CameraWidget` uses a `QGridLayout` capable of reshaping itself dynamically. When the user changes layouts (e.g., from 1x1 to 2x2), the widget securely unhooks active `VideoPlayerWidget`s, recreates the necessary grid cells, and repopulates the pickers.

### Intelligent Auto-Allocation
When building the grid or toggling streams on/off in the sidebar, the `autoAssignCameras()` routine runs. It scans all known *active* cameras and maps them iteratively into the earliest available `GridCell`. 

### Bandwidth Conservation On Tab Switch
The GUI optimizes bandwidth by subscribing to the `ui->tabWidget->currentChanged` signal in `MainWindow`. Whenever the user leaves a `CameraWidget` tab, the GUI calls `pauseAllStreams()` to locally halt video playback and inform the ROS backend to stop streaming. When the user rotates back to the tab, `resumeAllStreams()` kicks the feeds back on automatically.

### Sidebar Management
A collapsible sidebar (toggled via the ☰ button on the toolbar) displays an alphabetical list of all known cameras broadcast by the ROS system. It provides discrete Start/Stop buttons. Clicking these buttons sends a service request over ROS. Once the GUI sees the camera status change, it automatically maps the stream to the grid.

## 3. Keyboard Shortcuts

The GUI is designed to heavily favor keyboard-centric power users. There are global shortcuts (available anywhere) and view-specific shortcuts.

Press `/` anywhere in the application to pop up the Cheat Sheet overlay.

### Global Application Shortcuts (`MainWindow`)
| Shortcut | Action |
|----------|--------|
| `Ctrl + Tab` | Next Tab (cycles forward through the open views) |
| `Ctrl + Shift + Tab` | Previous Tab (cycles backward) |
| `Ctrl + T` | Open Add Tab Dialog (use arrow keys or 'C'/'J' hotkeys + Enter) |
| `Ctrl + W` | Close the currently focused tab |
| `Ctrl + Shift + Q` | Immediatly terminate the GUI application |
| `/` | Show keyboard shortcut cheat sheet overlay |

### Camera Widget Shortcuts (`CameraWidget`)
*Note: These shortcuts only respond when you are actively inside a Camera Tab.*
| Shortcut | Action |
|----------|--------|
| `Ctrl + 1` through `Ctrl + 9` | Toggles the Nth camera (based on sidebar alphabetical order) explicitly On/Off. |
| `Ctrl + A` | Batch toggles all cameras On. If all are currently On, it toggles them all Off. |
| `Ctrl + R` | Rapid-focus the Layout Dropdown box, allowing arrow-key selection of grid layouts (e.g. 2x2, 4x4). |

## 4. Build & Run Instructions

### Prerequisites
- ROS 2 (Humble or later)
- Qt 6 (Core, Gui, Widgets)
- A configured `kratos-rover` workspace

### Building
Navigate to the root of your workspace (e.g., `~/ros2_ws`) and run:
```bash
colcon build --packages-select kratos_gui
```

### Sourcing and Running
Source the workspace and run the node:
```bash
source install/setup.bash
ros2 run kratos_gui kratos_gui_exec
```

*Tip: For debugging Qt layouts visually, consider using the `QT_DEBUG_PLUGINS=1` environment variable.*
