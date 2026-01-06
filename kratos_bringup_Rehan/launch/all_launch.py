from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="gui_driveldarm_pkg",
                namespace="",
                executable="map.py",
                name="send_input",
            ),
            Node(
                package="joy",
                namespace="",
                executable="joy_node",
                name="joy",
                parameters=[
                    {
                        "device_name": "Sony Interactive Entertainment Wireless Controller"
                    }
                ],
            ),
            Node(
                package="joy",
                namespace="",
                executable="joy_node",
                name="joy0",
                parameters=[{"device_name": "Thrustmaster T.Flight Hotas One"}],
                remappings=[("/joy", "/joy0")],
            ),
            Node(
                package="gui_driveldarm_pkg",
                namespace="",
                executable="drive.py",
                name="drive_node",
            ),
        ]
    )
