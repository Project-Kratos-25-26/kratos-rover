from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory, get_package_prefix
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue
import os

def generate_launch_description():
    # Paths
    kratos_description_dir = get_package_share_directory('kratos_description')
    kratos_gazebo_dir = get_package_share_directory('kratos_gazebo')
    xacro_file = os.path.join(kratos_gazebo_dir, 'urdf', 'kratos.gazebo.xacro')
    world_file = os.path.join(kratos_gazebo_dir, 'worlds', 'test.world')
    
    # Set Gazebo model path
    pkg_prefix = get_package_prefix('kratos_description')
    gazebo_model_path = SetEnvironmentVariable(
        name='GAZEBO_MODEL_PATH',
        value=os.pathsep.join([
            os.environ.get('GAZEBO_MODEL_PATH', ''),
            os.path.join(pkg_prefix, 'share'),
            os.path.join(os.environ.get('HOME', ''), '.gazebo', 'models')
        ])
    )
    
    # Set Gazebo resource path
    gazebo_resource_path = SetEnvironmentVariable(
        name='GAZEBO_RESOURCE_PATH',
        value=os.pathsep.join([
            os.environ.get('GAZEBO_RESOURCE_PATH', ''),
            '/usr/share/gazebo',
            kratos_description_dir
        ])
    )
    
    # Launch Gazebo
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(get_package_share_directory('gazebo_ros'), 'launch', 'gazebo.launch.py')
        ),
        launch_arguments={'world': world_file, 'verbose': 'true'}.items()
    )
    
    # Robot State Publisher
    rsp_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': ParameterValue(
                Command(['xacro ', xacro_file]),
                value_type=str
            )
        }]
    )
    
    # Spawn robot in Gazebo
    spawn_node = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=[
            '-topic', 'robot_description',
            '-entity', 'kratos',
            '-x', '0.0',
            '-y', '0.0',
            '-z', '0.2'
        ],
        output='screen'
    )

    compass_node = Node(
        package='kratos_gazebo',
        executable='fake_mavros',
        name='fake_mavros_compass',
        output='screen'
    )

    return LaunchDescription([
        gazebo_model_path,
        gazebo_resource_path,
        gazebo_launch,
        rsp_node,
        spawn_node,
        compass_node
    ])

