#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy
from std_msgs.msg import Int32MultiArray

class InputMap(Node):
    
    def __init__(self):
        super().__init__("input_map")
        self.input_pub_=self.create_publisher(Int32MultiArray,"/motor",10)
        self.timer=self.create_timer(1.5,self.callback)

    def callback(self):
        msg=Int32MultiArray()
        msg.data=[1,2,3,4,5,6,7,8,9,10,11,12]
        # msg.data=abs(1-msg.data)
        self.input_pub_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    Map=InputMap()
    rclpy.spin(Map)
    rclpy.shutdown()


if __name__=='__main__':
    main()