#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64
import math

def quaternion_to_yaw(x, y, z, w):
    t3 = +2.0 * (w * z + x * y)
    t4 = +1.0 - 2.0 * (y * y + z * z)
    return math.atan2(t3, t4)

class FakeMavrosCompass(Node):
    def __init__(self):
        super().__init__('fake_mavros_compass')
        
        # INPUT: Use Sensor Data QoS (Best Effort) to ensure we hear Gazebo
        self.subscription = self.create_subscription(
            Odometry,
            '/odom',
            self.listener_callback,
            qos_profile_sensor_data) # <--- Critical Fix
            
        # OUTPUT: Use Reliable (10) for your Mission Manager
        self.publisher_ = self.create_publisher(Float64, '/mavros/global_position/compass_hdg', 10)
        
        self.get_logger().info("Compass Bridge Started. Waiting for /odom...")

    def listener_callback(self, msg):
        q = msg.pose.pose.orientation
        ros_yaw = math.degrees(quaternion_to_yaw(q.x, q.y, q.z, q.w))
        
        compass_heading = (-ros_yaw) % 360.0

        out_msg = Float64()
        out_msg.data = compass_heading
        self.publisher_.publish(out_msg)

def main(args=None):
    rclpy.init(args=args)
    node = FakeMavrosCompass()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
