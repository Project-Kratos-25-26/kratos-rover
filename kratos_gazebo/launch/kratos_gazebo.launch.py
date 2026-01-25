import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory, get_package_prefix
from launch.substitutions import Command
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():
    
    # ---------------------------------------------------------
    # 1. Path Configuration
    # ---------------------------------------------------------
    kratos_description_dir = get_package_share_directory('kratos_description')
    kratos_gazebo_dir = get_package_share_directory('kratos_gazebo')
    
    # Paths to files
    xacro_file = os.path.join(kratos_gazebo_dir, 'urdf', 'kratos.gazebo.xacro')
    world_file = os.path.join(kratos_gazebo_dir, 'worlds', 'mars.world')
    
    # ---------------------------------------------------------
    # 2. Environment Variables
    # ---------------------------------------------------------
    # Using your preferred logic, but cleaned up for robustness
    
    pkg_description_prefix = get_package_prefix('kratos_description')
    pkg_gazebo_prefix = get_package_prefix('kratos_gazebo')
    
    # GAZEBO_MODEL_PATH:
    # Includes /usr/share/gazebo-11/models for default objects (sun, ground plane)
    # Includes workspace share folders so "package://" works
    model_path_value = os.pathsep.join([
        os.environ.get('GAZEBO_MODEL_PATH', ''),
        os.path.join(pkg_description_prefix, 'share'),
        os.path.join(pkg_gazebo_prefix, 'share'),
        '/usr/share/gazebo-11/models',
        os.path.join(os.environ.get('HOME', ''), '.gazebo', 'models')
    ])
    
    # GAZEBO_RESOURCE_PATH:
    # Includes specific package directories for meshes and world textures
    resource_path_value = os.pathsep.join([
        os.environ.get('GAZEBO_RESOURCE_PATH', ''),
        '/usr/share/gazebo-11',
        kratos_description_dir, 
        kratos_gazebo_dir       
    ])

    gazebo_model_path = SetEnvironmentVariable(name='GAZEBO_MODEL_PATH', value=model_path_value)
    gazebo_resource_path = SetEnvironmentVariable(name='GAZEBO_RESOURCE_PATH', value=resource_path_value)
    
    # ---------------------------------------------------------
    # 3. Launch Gazebo (Paused Mode)
    # ---------------------------------------------------------
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(get_package_share_directory('gazebo_ros'), 'launch', 'gazebo.launch.py')
        ),
        launch_arguments={
            'world': world_file,
            'verbose': 'true',
            'paused': 'true'  # <--- Starts paused so you can check spawn location
        }.items()
    )
    
    # ---------------------------------------------------------
    # 4. Robot State Publisher
    # ---------------------------------------------------------
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
    
    # ---------------------------------------------------------
    # 5. Spawn Robot
    # ---------------------------------------------------------
    spawn_node = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=[
            '-topic', 'robot_description',
            '-entity', 'kratos',
            '-x', '6.0',
            '-y', '5.0',
            '-z', '5',   # <--- Fixed typo (was '50 .0')
            '-timeout', '120.0'
        ],
        output='screen'
    )

    # ---------------------------------------------------------
    # 6. Compass
    # ---------------------------------------------------------
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