#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from slam_toolbox.srv import SerializePoseGraph


class AutoSerializeMap(Node):
    def __init__(self):
        super().__init__('auto_serialize_map')

        self.declare_parameter('map_path', '')
        self.declare_parameter('period', 10.0)
        self.declare_parameter('start_delay', 2.0)

        self.map_path = self.get_parameter('map_path').value
        self.period = self.get_parameter('period').value
        self.start_delay = self.get_parameter('start_delay').value

        if not self.map_path:
            self.get_logger().error("map_path parameter is required")
            return

        self.client = self.create_client(
            SerializePoseGraph,
            '/slam_toolbox/serialize_posegraph'
        )

        self.get_logger().info("Waiting for serialize_posegraph service...")

        self.serialize_timer = self.create_timer(
            self.period,
            self.serialize_map
        )
        self.serialize_timer.cancel()

        self.start_timer = self.create_timer(
            self.start_delay,
            self.start_serialization
        )

    def start_serialization(self):
        if not self.client.service_is_ready():
            self.get_logger().info("Service not ready yet...")
            return

        self.get_logger().info(
            f"Starting auto serialization every {self.period}s"
        )
        self.start_timer.cancel()
        self.serialize_timer.reset()

    def serialize_map(self):
        if not self.client.service_is_ready():
            self.get_logger().warn("serialize_posegraph service not ready")
            return

        req = SerializePoseGraph.Request()
        req.filename = self.map_path
        req.serialize_data = True

        future = self.client.call_async(req)
        future.add_done_callback(self.serialize_done)

    def serialize_done(self, future):
        try:
            result = future.result()
            if result.result == 0:
                self.get_logger().info("Map serialized successfully")
            else:
                self.get_logger().error("Map serialization failed")
        except Exception as e:
            self.get_logger().error(str(e))


def main():
    rclpy.init()
    node = AutoSerializeMap()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
