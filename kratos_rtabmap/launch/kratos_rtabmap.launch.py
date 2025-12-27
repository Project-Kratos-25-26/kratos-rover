import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from launch.actions import OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch.conditions import UnlessCondition

parameters = []
remappings = []

def launch_setup(context: LaunchContext, *args, **kwargs):

    parameters = [{
        'use_sim_time': True,
        'frame_id': 'base_link',
        'odom_frame_id': 'odom',        # 🔴 REQUIRED
        'map_frame_id': 'map',          # good practice
        'publish_tf': True,
        'subscribe_rgbd': True,
        'approx_sync': True
    }]

    remappings = [
        ('imu', '/zed/zed_node/imu/data')
    ]

    if LaunchConfiguration('use_zed_odometry').perform(context) in ["True", "true"]:
        remappings.append(('odom', '/zed/zed_node/odom'))
    else:
        parameters.append({'subscribe_odom_info': True})

    return [

        # RGB-D sync
        Node(
            package='rtabmap_sync',
            executable='rgbd_sync',
            output='screen',
            parameters=parameters,
            remappings=[
                ('rgb/image', '/zed/zed_node/left/image_rect_color'),
                ('rgb/camera_info', '/zed/zed_node/left/camera_info'),
                ('depth/image', '/zed/zed_node/depth/depth_registered')
            ]
        ),

        # Visual odometry
        Node(
            package='rtabmap_odom',
            executable='rgbd_odometry',
            output='screen',
            condition=UnlessCondition(LaunchConfiguration('use_zed_odometry')),
            parameters=parameters,
            remappings=remappings,
        ),

        # SLAM
        Node(
            package='rtabmap_slam',
            executable='rtabmap',
            output='screen',
            parameters=parameters,
            remappings=remappings,
            arguments=['-d']
        ),

        # Visualization
        Node(
            package='rtabmap_viz',
            executable='rtabmap_viz',
            output='screen',
            parameters=parameters,
            remappings=remappings
        )
    ]


def generate_launch_description():
    return LaunchDescription([
        # Launch arguments
        DeclareLaunchArgument(
            'use_zed_odometry', 
            default_value='false',
            description='Use zed\'s computed odometry instead of using rtabmap\'s odometry.'
        ),
        
        OpaqueFunction(function=launch_setup)
    ])