#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from slam_toolbox.srv import SerializePoseGraph


class LoadMapNode(Node):
    def __init__(self):
        super().__init__('load_map_on_start')

        self.declare_parameter('map_path', '')
        self.map_path = self.get_parameter('map_path').value

        if not self.map_path:
            self.get_logger().error("map_path parameter is required")
            return

        self.client = self.create_client(
            SerializePoseGraph,
            '/slam_toolbox/serialize_posegraph'
        )

        self.timer = self.create_timer(1.0, self.try_load_map)

    def try_load_map(self):
        if not self.client.service_is_ready():
            self.get_logger().info("Waiting for serialize_posegraph service...")
            return

        self.timer.cancel()

        req = SerializePoseGraph.Request()
        req.filename = self.map_path
        req.serialize_data = False  # False = LOAD

        future = self.client.call_async(req)
        future.add_done_callback(self.load_done)

    def load_done(self, future):
        try:
            result = future.result()
            if result.result == 0:
                self.get_logger().info(
                    f"Map loaded successfully from {self.map_path}"
                )
            else:
                self.get_logger().error("Failed to load map")
        except Exception as e:
            self.get_logger().error(str(e))


def main():
    rclpy.init()
    node = LoadMapNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
