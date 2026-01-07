import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition

def launch_setup(context: LaunchContext, *args, **kwargs):
    
    # Check if we are using visual odometry or wheel odometry
    use_visual_odom = LaunchConfiguration('visual_odometry').perform(context).lower() in ['true', '1']
    
    # Common Parameters
    parameters = [{
        'use_sim_time': True,
        'frame_id': 'base_link',
        'map_frame_id': 'map',
        'publish_tf': True,
        'subscribe_rgbd': True,
        'approx_sync': True,
        'wait_imu_to_init': True,
        'qos': 2,
        'Optimizer/Slam2D': 'true',
        'Reg/Force3DoF': 'true',
        # RTAB-Map Optimization parameters (optional but good for sim)
        'Reg/Strategy': '0',       # 0=Visual, 1=ICP, 2=Visual+ICP
        'RGBD/ProximityBySpace': 'false',
    }]

    # Define Remappings
    # We map the inputs to your Simulation topics
    common_remappings = [
        ('rgb/image', '/zed/zed_node/left/image_rect_color'),
        ('rgb/camera_info', '/zed/zed_node/left/camera_info'),
        ('depth/image', '/zed/zed_node/depth/depth_registered'),
        ('imu', '/zed/zed_node/imu/data')
    ]

    # LOGIC: 
    # If Visual Odom is OFF, we must listen to Wheel Odom (/odom)
    # If Visual Odom is ON, the rgbd_odometry node publishes to /odom (internally)
    if not use_visual_odom:
        parameters[0]['subscribe_odom_info'] = False # Wheel odom has no info covariance usually
        common_remappings.append(('odom', '/odom')) # Connect to Diff Drive Controller

    return [
        # 1. RGB-D Sync Node
        # Compresses RGB + Depth into a single "RGBDImage" message for RTAB-Map
        Node(
            package='rtabmap_sync',
            executable='rgbd_sync',
            output='screen',
            parameters=parameters,
            remappings=common_remappings
        ),

        # 2. Visual Odometry (Only runs if visual_odometry:=true)
        # We generally KEEP THIS OFF for your simulation
        Node(
            package='rtabmap_odom',
            executable='rgbd_odometry',
            output='screen',
            condition=IfCondition(LaunchConfiguration('visual_odometry')),
            parameters=parameters,
            remappings=common_remappings
        ),

        # 3. RTAB-Map SLAM ( The Brain )
        Node(
            package='rtabmap_slam',
            executable='rtabmap',
            output='screen',
            parameters=parameters,
            remappings=common_remappings,
            arguments=['-d'] # Delete database on start
        ),

        # 4. Visualization (RTAB-Map Viz)
        Node(
            package='rtabmap_viz',
            executable='rtabmap_viz',
            output='screen',
            parameters=parameters,
            remappings=common_remappings
        )
    ]

def generate_launch_description():
    return LaunchDescription([
        # Argument to switch between Visual Odom (Cameras) and Wheel Odom (Encoders)
        DeclareLaunchArgument(
            'visual_odometry', 
            default_value='false',
            description='If true, computes odometry from camera. If false, uses /odom topic.'
        ),
        
        OpaqueFunction(function=launch_setup)
    ])
