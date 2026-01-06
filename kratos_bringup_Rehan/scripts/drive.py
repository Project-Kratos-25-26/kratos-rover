#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32MultiArray
from sensor_msgs.msg import Joy
import math

class Drive(Node):
    def __init__(self):
        super().__init__('drive_node')
        
        # Initialize variables
        self.rover_x = 0
        self.rover_z = 0
        
        # Create subscription to joystick
        self.subscription = self.create_subscription(
            Joy,
            '/joy0',
            self.callback,
            10
        )
        
        # Create publisher for rover commands
        self.values = Float32MultiArray()
        self.actualValues = [0.0] * 6  # Initialize with floats instead of integers
        self.values.data = self.actualValues
        self.publisher = self.create_publisher(
            Float32MultiArray,
            '/rover',
            10
        )
        
        # Create timer for periodic publishing
        self.timer = self.create_timer(0.1, self.timer_callback)  # 10Hz rate

    def timer_callback(self):
        self.actualValues = self.move()
        self.values.data = self.actualValues
        self.publisher.publish(self.values)

    def teleop(self, linear, rotational, speed):
        self.rover_x = (linear*((speed+1)/2) + rotational*((speed+1)/2))*100
        self.rover_z = (linear*((speed+1)/2) - rotational*((speed+1)/2))*100

        # Calculate magnitude of the vector
        magnitude = math.sqrt(self.rover_x**2 + self.rover_z**2)

        # Map to circular space if magnitude exceeds 100
        if magnitude > 100:
            scale_factor = 100 / magnitude
            self.rover_x *= scale_factor
            self.rover_z *= scale_factor

    def callback(self, msg):
        if msg.axes[1] > 0.1 or msg.axes[1] < -0.1 or msg.axes[0] > 0.1 or msg.axes[0] < -0.1:
            self.teleop(msg.axes[1], msg.axes[0], msg.axes[2])
        else:
            self.rover_x = 0
            self.rover_z = 0

    def move(self):
        velx = self.rover_x * 0.7
        velz = self.rover_z * 0.7

        print(velx, " ", velz)

        if (velx >= 0 and velz >= 0):
            left_wheel_front = (((velx+velz)/2+(velx-velz)/2)*100.0)/151.0
            right_wheel_front = (((velx+velz)/2-(velx-velz)/2)*100.0)/144.0
            right_wheel_mid = (((velx+velz)/2+(velx-velz)/2)*100.0)/147.5
            left_wheel_mid = (((velx+velz)/2-(velx-velz)/2)*100.0)/158.0
            right_wheel_back = (((velx+velz)/2+(velx-velz)/2)*100.0)/76.0
            left_wheel_back = (((velx+velz)/2-(velx-velz)/2)*100.0)/164.0
        
        elif (velx <= 0 and velz <= 0):
            right_wheel_front = ((velx+velz)/2+(velx-velz)/2) * (100.0/148.0)
            left_wheel_front = ((velx+velz)/2-(velx-velz)/2) * (100.0/147.3)
            right_wheel_mid = ((velx+velz)/2+(velx-velz)/2) * (100.0/151.4)
            left_wheel_mid = ((velx+velz)/2-(velx-velz)/2) * (100.0/152.35)
            right_wheel_back = ((velx+velz)/2+(velx-velz)/2) * (100.0/86.0)
            left_wheel_back = ((velx+velz)/2-(velx-velz)/2) * (100.0/149.2)
        
        elif (velx >= 0 and velz <= 0):
            right_wheel_front = ((velx+velz)/2+(velx-velz)/2) * (100.0/147.3)
            left_wheel_front = ((velx+velz)/2-(velx-velz)/2) * (100.0/144.0)
            right_wheel_mid = ((velx+velz)/2+(velx-velz)/2) * (60.0/147.5)
            left_wheel_mid = ((velx+velz)/2-(velx-velz)/2) * (60.0/152.35)
            right_wheel_back = ((velx+velz)/2+(velx-velz)/2) * (100.0/76.0)
            left_wheel_back = ((velx+velz)/2-(velx-velz)/2) * (100.0/149.2)
         
        elif (velx <= 0 and velz >= 0):
            right_wheel_front = ((velx+velz)/2+(velx-velz)/2) * (100.0/151.0)
            left_wheel_front = ((velx+velz)/2-(velx-velz)/2) * (100.0/148.0)
            right_wheel_mid = ((velx+velz)/2+(velx-velz)/2) * (60.0/151.4)
            left_wheel_mid = ((velx+velz)/2-(velx-velz)/2) * (60.0/158.0)
            right_wheel_back = ((velx+velz)/2+(velx-velz)/2) * (100.0/86.0)
            left_wheel_back = ((velx+velz)/2-(velx-velz)/2) * (100.0/164.0)
         
        else:  # velx == 0 and velz == 0
            right_wheel_front = 0
            left_wheel_front = 0
            right_wheel_mid = 0
            left_wheel_mid = 0
            right_wheel_back = 0
            left_wheel_back = 0

        # List of all wheel speeds
        wheel_speeds = [
            right_wheel_front,
            left_wheel_front,
            right_wheel_mid,
            left_wheel_mid,
            right_wheel_back,
            left_wheel_back,
        ]

        # Find the maximum absolute speed
        max_speed = max(abs(speed) for speed in wheel_speeds)

        # If the maximum speed exceeds the allowed limit
        if max_speed > 70:
            scale_factor = 70 / max_speed  # Calculate scale factor
            wheel_speeds = [speed * scale_factor for speed in wheel_speeds]

            # Update individual wheel variables
            right_wheel_front, left_wheel_front, right_wheel_mid, left_wheel_mid, right_wheel_back, left_wheel_back = wheel_speeds

        return right_wheel_front, left_wheel_front, right_wheel_mid, left_wheel_mid, right_wheel_back, left_wheel_back


def main(args=None):
    rclpy.init(args=args)
    drive_node = Drive()
    
    try:
        rclpy.spin(drive_node)
    except KeyboardInterrupt:
        pass
    finally:
        drive_node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()