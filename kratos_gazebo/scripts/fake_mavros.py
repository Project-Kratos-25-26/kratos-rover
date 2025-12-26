#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from std_msgs.msg import Float64
import math

def quaternion_to_yaw(x, y, z, w):
    """
    Convert Quaternion to Yaw (Rotation around Z axis)
    """
    t3 = +2.0 * (w * z + x * y)
    t4 = +1.0 - 2.0 * (y * y + z * z)
    return math.atan2(t3, t4)

class FakeMavrosCompass(Node):
    def __init__(self):
        super().__init__('fake_mavros_compass')
        
        # Subscribe to Gazebo's Ground Truth (Odometry)
        self.subscription = self.create_subscription(
            Odometry,
            '/odom',  # Ensure this matches your diff_drive output topic
            self.listener_callback,
            10)
            
        # Publisher for the specific MAVROS topic
        self.publisher_ = self.create_publisher(
            Float64, 
            '/mavros/global_position/compass_hdg', 
            10)

    def listener_callback(self, msg):
        # 1. Extract Quaternion
        q = msg.pose.pose.orientation
        
        # 2. Convert to Yaw (Radians: -Pi to +Pi)
        yaw_rad = quaternion_to_yaw(q.x, q.y, q.z, q.w)
        
        # 3. Convert to Compass Heading (Degrees: 0 to 360)
        # 0 = North (Forward x), 90 = East, etc.
        # Note: MAVROS compass usually treats 0 as North (Positive X in Sim)
        heading_deg = math.degrees(yaw_rad)
        
        if heading_deg < 0:
            heading_deg += 360.0
            
        # 4. Publish
        out_msg = Float64()
        out_msg.data = heading_deg
        self.publisher_.publish(out_msg)

def main(args=None):
    rclpy.init(args=args)
    node = FakeMavrosCompass()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
