import os
import tempfile

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription, LaunchContext
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def launch_setup(context: LaunchContext, *args, **kwargs):

    # -----------------------------
    # ZED parameter override file
    # -----------------------------
    with tempfile.NamedTemporaryFile(mode='w+t', delete=False) as zed_override_file:
        zed_override_file.write(
            "---\n"
            "/**:\n"
            "  ros__parameters:\n"
            "    general:\n"
            "      grab_resolution: 'HD720'\n"
        )
        zed_override_file.flush()

        use_zed_odom = True

        # -----------------------------
        # Common RTAB-Map parameters
        # -----------------------------
        common_params = {
            'frame_id': 'zed_camera_link',
            'odom_frame_id': 'odom',
            'map_frame_id': 'map',
            'publish_tf': True,
            'subscribe_rgbd': True,
            'subscribe_odom': True,
            'wait_imu_to_init': True,
            'queue_size': 20,
            'sync_queue_size': 20,
            'topic_queue_size': 20,
            'Odom/Strategy': '0'  # Frame-to-Map
        }

        # -----------------------------
        # Topic remappings
        # -----------------------------
        rtabmap_remappings = [
            ('imu', '/zed/zed_node/imu/data'),
            ('odom', '/zed/zed_node/odom') 
        ]

        if use_zed_odom:
            rtabmap_remappings.append(('odom', '/zed/zed_node/odom'))

        # -----------------------------
        # Nodes
        # -----------------------------
        nodes = [

            # ---- ZED Camera ----
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(
                        get_package_share_directory('zed_wrapper'),
                        'launch',
                        'zed_camera.launch.py'
                    )
                ),
                launch_arguments={
                    'camera_model': LaunchConfiguration('camera_model'),
                    'ros_params_override_path': zed_override_file.name,
                    'publish_tf': 'true',
                    'publish_map_tf': 'true',
                    'publish_imu_tf': 'true',
                    'sensors.publish_imu': 'true'
                }.items(),
            ),

            # ---- RGB-D Sync (NO approx sync for ZED) ----
            Node(
                package='rtabmap_sync',
                executable='rgbd_sync',
                name='rgbd_sync',
                output='screen',
                parameters=[{
                    'approx_sync': False,
                    'queue_size': 20,
                    'sync_queue_size': 20,
                    'topic_queue_size': 20
                }],
                remappings=[
                    ('rgb/image', '/zed/zed_node/left/image_rect_color'),
                    ('rgb/camera_info', '/zed/zed_node/left/camera_info'),
                    ('depth/image', '/zed/zed_node/depth/depth_registered')
                ]
            ),

            # ---- RTAB-Map SLAM ----
            Node(
                package='rtabmap_slam',
                executable='rtabmap',
                name='rtabmap',
                output='screen',
                parameters=[common_params],
                remappings=rtabmap_remappings,
                arguments=['-d']
            ),

            # ---- RTAB-Map Visualization ----
            Node(
                package='rtabmap_viz',
                executable='rtabmap_viz',
                name='rtabmap_viz',
                output='screen',
                parameters=[common_params],
                remappings=rtabmap_remappings
            ),
        ]

        return nodes


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'camera_model',
            default_value='zed2i',
            description='ZED camera model'
        ),
        OpaqueFunction(function=launch_setup)
    ])
