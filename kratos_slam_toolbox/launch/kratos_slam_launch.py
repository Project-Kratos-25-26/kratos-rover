import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    package_dir = get_package_share_directory('kratos_slam_toolbox')
    slam_toolbox_dir = get_package_share_directory('slam_toolbox')

    map_path = os.path.join(package_dir, 'maps', 'auto_map')

    return LaunchDescription([

        # ---------------- SLAM TOOLBOX ----------------
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(
                    slam_toolbox_dir,
                    'launch',
                    'online_async_launch.py'
                )
            ),
            launch_arguments={
                'slam_params_file': os.path.join(
                    package_dir,
                    'config',
                    'mapper_params_online_async.yaml'
                )
            }.items()
        ),

        # -------- LOAD MAP AFTER SLAM TOOLBOX IS READY --------
        TimerAction(
            period=5.0,  # IMPORTANT: let slam_toolbox fully initialize
            actions=[
                Node(
                    package='kratos_slam_toolbox',
                    executable='load_map_on_start.py',
                    name='load_map_on_start',
                    output='screen',
                    parameters=[{
                        'map_path': map_path
                    }]
                )
            ]
        ),

        # -------- AUTO SERIALIZE MAP PERIODICALLY --------
        TimerAction(
            period=8.0,
            actions=[
                Node(
                    package='kratos_slam_toolbox',
                    executable='auto_serialize_map.py',
                    name='auto_serialize_map',
                    output='screen',
                    parameters=[{
                        'map_path': map_path,
                        'period': 10.0,
                        'start_delay': 2.0
                    }]
                )
            ]
        ),
    ])
