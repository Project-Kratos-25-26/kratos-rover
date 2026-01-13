#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from nav_msgs.msg import OccupancyGrid
from rclpy.qos import QoSProfile, DurabilityPolicy

class EmptyMapPublisher(Node):
    def __init__(self):
        super().__init__('empty_map_publisher')
        
        # Create QoS profile for latching behavior (like map_server)
        qos_profile = QoSProfile(depth=1)
        qos_profile.durability = DurabilityPolicy.TRANSIENT_LOCAL

        self.publisher_ = self.create_publisher(OccupancyGrid, 'map', qos_profile)
        
        # Publish timer (1 Hz, though latching handles late joiners)
        self.timer = self.create_timer(1.0, self.timer_callback)
        
        self.get_logger().info('Empty Map Publisher started')

    def timer_callback(self):
        msg = OccupancyGrid()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'map'
        
        # Map metadata
        msg.info.resolution = 0.05  # 5cm per pixel
        msg.info.width = 400
        msg.info.height = 400
        
        # Set origin so (0,0) is in the center
        # width * resolution / 2 = 400 * 0.05 / 2 = 10.0 meters offset
        msg.info.origin.position.x = -10.0
        msg.info.origin.position.y = -10.0
        msg.info.origin.position.z = 0.0
        msg.info.origin.orientation.w = 1.0
        
        # Empty map data (0 = free space)
        # -1 is unknown, 0 is free, 100 is occupied
        msg.data = [0] * (msg.info.width * msg.info.height)
        
        self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = EmptyMapPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
