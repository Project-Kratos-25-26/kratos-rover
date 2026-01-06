#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy
from std_msgs.msg import Int32MultiArray
from rclpy.callback_groups import ReentrantCallbackGroup,MutuallyExclusiveCallbackGroup
from rclpy.executors import  MultiThreadedExecutor

min_en1=286.7
min_en2=200.5
input_sub=ReentrantCallbackGroup()
input_sub1=ReentrantCallbackGroup()

class InputMap(Node):
    
    def __init__(self):
        super().__init__("input_map")
        self.angles=[0.0,0.0,0.0]
        self.callibrated_angles=Int32MultiArray()
        self.input_sub=self.create_subscription(Joy,"/joy",self.send_input,10,callback_group=input_sub)
        self.input_pub_=self.create_publisher(Int32MultiArray,"/arm_msg",10)
        self.feedback_sub=self.create_subscription(Int32MultiArray,"/arm_encoders",self.read_encoders,10,callback_group=input_sub1)
        self.callibrated_pub=self.create_publisher(Int32MultiArray,"/callibrated_angles",10)

    def read_encoders(self,fb:Int32MultiArray):
        self.angles[1]=fb.data[1]
        self.angles[2]=fb.data[2]
        self.callibrated_angles.data=[self.angles[1]-min_en1,self.angles[2]-min_en2]
        self.callibrated_pub.publish(self.callibrated_angles)

    def send_input(self,joy:Joy):
        '''
           Right stick: end effector pitch + roll
           Left stick : shoulder+elbow
           Lb Rb      : base yaw
           A B / X O       : gripper lead screw
        '''

        mapped_input=Int32MultiArray()
        mapped_input.data=[0,0,0,0,0,0,0,0,0,0,0,0]
        #[lin1_dir,lin2_dir,base_yaw,bevel1_dir,bevel2_dir,end_effector_dir
        # lin1_pwm,lin2_pwm,base_yaw_pwm,bevel1_pwm,
        # bevel2_pwm,empty(end_effector has constant speed)]


        #linear actuators
        for i in range(2):

            if(joy.axes[i]<=0):mapped_input.data[i]=1
            else: mapped_input.data[i]=0

            mapped_input.data[i+6]=int(((abs(joy.axes[i]))/1)*255)

        #bevel controls
        for i in range(3,5):
    
            if(joy.axes[i]<=0):mapped_input.data[i]=1
            else: mapped_input.data[i]=0
            mapped_input.data[i+6]=int(((abs(joy.axes[i]))/1)*85)
        
        # if(mapped_input.data[9]>=mapped_input.data[10]):
        #     mapped_input.data[4]=mapped_input.data[3]
        #     mapped_input.data[10]=mapped_input.data[9]
        # else:
        #     mapped_input.data[3]=mapped_input.data[4]
        #     mapped_input.data[4]= not mapped_input.data[4]
        #     mapped_input.data[9]=mapped_input.data[10]
    


        #base yaw controls
        normalised_lb= 0.5-(joy.axes[2]/2.0)
        normalised_rb= 0.5-(joy.axes[5]/2.0)

        if(normalised_lb>0 and normalised_rb>0):
            mapped_input.data[8]=0
        else:
            if normalised_lb>0:
                mapped_input.data[2]=0
                mapped_input.data[8]=int(normalised_lb*255)
            elif normalised_rb>0:
                mapped_input.data[2]=1
                mapped_input.data[8]=int(normalised_rb*255)


        #end effector buttons control
        mapped_input.data[5]=-1
        if(joy.buttons[0]==1 and joy.buttons[1]==1):
            mapped_input.data[5]=-1 
        else:
            if(joy.buttons[0]==1):
                mapped_input.data[5]=0
            if(joy.buttons[1]==1):
                mapped_input.data[5]=1       


        self.input_pub_.publish(mapped_input)

        if(joy.buttons[2]==1 and joy.buttons[3]==1):
            self.callibrate()

    def callibrate(self):

        min_en1=360
        min_en2=360
        msg=Int32MultiArray()
        while(abs(self.angles[0]-min_en1)>=2):
            msg.data=[1,0,0,0,0,0,125,0,0,0,0,0]
            self.input_pub_.publish(msg)
            min_en1=self.angles[1]
            print("min_en1: "+str(min_en1))
        
        msg.data=[0,0,0,0,0,0,0,0,0,0,0,0]

        self.input_pub_.publish(msg)

        while(abs(self.angles[1]-min_en2)>=2):
            msg.data=[0,1,0,0,0,0,0,125,0,0,0,0]
            self.input_pub_.publish(msg)
            min_en2=self.angles[2]
            print("min_en2: "+str(min_en2))

        msg.data=[0,0,0,0,0,0,0,0,0,0,0,0]

        self.input_pub_.publish(msg)



          

    
def main(args=None):
    rclpy.init(args=args)
    Map=InputMap()
    executor=MultiThreadedExecutor()
    executor.add_node(Map)
    executor.spin()
    Map.destroy_node()
    rclpy.shutdown()


if __name__=='__main__':
    main()