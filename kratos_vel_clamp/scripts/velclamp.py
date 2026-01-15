#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist


class AngularClamp(Node):
    def __init__(self):
        super().__init__('angular_clamp')

        # --- Tunable parameters ---
        self.min_w = 0.8       # minimum angular velocity (rad/s)
        self.max_w = 1.5        # maximum angular velocity (rad/s)
        self.deadband = 0.05    # ignore noise around zero
        self.w_in_max = 2.0     # expected max |angular.z| from upstream

        # --- ROS interfaces ---
        self.sub = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_cb,
            10
        )

        self.pub = self.create_publisher(
            Twist,
            '/cmd_vel_filt',
            10
        )

        self.get_logger().info('AngularClamp node started')

    def cmd_vel_cb(self, msg: Twist):
        out = Twist()

        # Pass linear velocity through unchanged
        out.linear = msg.linear

        w = msg.angular.z

        # Deadband handling
        if abs(w) < self.deadband:
            out.angular.z = 0.0
        else:
            # Normalize input angular velocity to [0, 1]
            w_norm = min(abs(w) / self.w_in_max, 1.0)

            # Scale into [min_w, max_w]
            mag = self.min_w + w_norm * (self.max_w - self.min_w)

            # Restore direction
            out.angular.z = mag if w > 0.0 else -mag

        self.pub.publish(out)


def main():
    rclpy.init()
    node = AngularClamp()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
