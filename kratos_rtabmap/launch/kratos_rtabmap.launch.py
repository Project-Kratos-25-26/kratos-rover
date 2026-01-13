import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.conditions import UnlessCondition
import tempfile

def launch_setup(context: LaunchContext, *args, **kwargs):
    with tempfile.NamedTemporaryFile(mode='w+t', delete=False) as zed_override_file:
        zed_override_file.write("---\n" +
                                "/**:\n" +
                                "    ros__parameters:\n" +
                                "        general:\n" +
                                "            grab_resolution: 'HD720'")
        
        # use_zed_odom = LaunchConfiguration('use_zed_odometry').perform(context) in ["True", "true"]
        use_zed_odom = True
        # Common parameters
        common_params = {
            'frame_id': 'zed_camera_link',        # robot base
   	    'odom_frame_id': 'odom',        # local odometry
            'map_frame_id': 'map',          # global map
            'publish_tf': True,    
            'subscribe_rgbd': True,
            'approx_sync': True,
            'wait_imu_to_init': True,
            'queue_size': 50,
            'sync_queue_size': 50,      # Increased from default 30
            'topic_queue_size': 50,     # Increased from default 10
            'Odom/Strategy': '0'  # 0=Frame-to-Map, 1=Frame-to-Frame
        }
        
        # Remappings for rgbd_odometry
        odom_remappings = [
            ('imu', '/zed/zed_node/imu/data'),
            ('rgbd_image', '/rgbd_image')
        ]
        
        # Remappings for rtabmap and viz
        rtabmap_remappings = [
            ('imu', '/zed/zed_node/imu/data')
        ]
        
        if use_zed_odom:
            rtabmap_remappings.append(('odom', '/zed/zed_node/odom'))
        else:
            # rgbd_odometry will publish to /odom by default
            pass
        
        nodes = [
            # Launch camera driver
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource([os.path.join(
                    get_package_share_directory('zed_wrapper'), 'launch'),
                    '/zed_camera.launch.py']),
                launch_arguments={
                    'camera_model': LaunchConfiguration('camera_model'),
                    'ros_params_override_path': zed_override_file.name,
                    'publish_tf': 'true',
                    'publish_map_tf': 'true',
                    'publish_imu_tf': 'true',
                    'sensors.publish_imu': 'true'
                }.items(),
            ),
            
            # Sync rgb/depth/camera_info together
            Node(   
                package='rtabmap_sync',
                executable='rgbd_sync',
                output='screen',
                parameters=[common_params],
                remappings=[
                    ('rgb/image', '/zed/zed_node/left/image_rect_color'),
                    ('rgb/camera_info', '/zed/zed_node/left/camera_info'),
                    ('depth/image', '/zed/zed_node/depth/depth_registered')
                ]
            ),
        ]
        
        # Add rgbd_odometry only when NOT using ZED odometry
        if not use_zed_odom:
            nodes.append(
                Node(
                    package='rtabmap_odom',
                    executable='rgbd_odometry',
                    output='screen',
                    parameters=[common_params],
                    remappings=odom_remappings,
                )
            )
        
        # Add rtabmap
        nodes.append(
            Node(
                package='rtabmap_slam',
                executable='rtabmap',
                output='screen',
                parameters=[common_params],
                remappings=rtabmap_remappings,
                arguments=['-d']
            )
        )
        
        # Add visualization
        nodes.append(
            Node(
                package='rtabmap_viz',
                executable='rtabmap_viz',
                output='screen',
                parameters=[common_params],
                remappings=rtabmap_remappings
            )
        )
        
        return nodes

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'use_zed_odometry',
            default_value='true',
            description='Use ZED odometry'
        ),
        DeclareLaunchArgument(
            'camera_model',
            default_value='zed2i',
            description='ZED camera model'
        ),
        OpaqueFunction(function=launch_setup)
    ])
