#include "ros_worker.h"

#include <chrono>

RosWorker::RosWorker(QObject *parent) : QObject(parent) {
  // register camera list type so we can emit it across threads
  qRegisterMetaType<QPair<QString,int>>("QPair<QString,int>");
  qRegisterMetaType<QList<QPair<QString,int>>>("QList<QPair<QString,int>>");

  // Create a ROS 2 node called "gui_vibecoded_node"
  node_ = rclcpp::Node::make_shared("gui_vibecoded_node");

  // Subscribe to the "/foo" topic (type: std_msgs/msg/String)
  sub_ = node_->create_subscription<std_msgs::msg::String>(
      "/foo",
      10, // QoS depth
      [this](std_msgs::msg::String::SharedPtr msg) {
        QString data = QString::fromStdString(msg->data);
        QStringList parts = data.split(" | ");

        double latencyMs = 0.0;
        QString cleanMsg = data;

        if (parts.size() == 2) {
          cleanMsg = parts[0];
          bool ok;
          double sentTime = parts[1].toDouble(&ok);
          if (ok) {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            double nowSeconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(duration)
                    .count() /
                1000.0;
            latencyMs = (nowSeconds - sentTime) * 1000.0;
            if (latencyMs < 0) {
              latencyMs = 0.0; // Clock sync issue
            }
          }
        }

        // Forward the payload and latency to Qt land via a signal
        emit messageReceived(cleanMsg, latencyMs);
      });

  RCLCPP_INFO(node_->get_logger(), "Subscribed to /foo");

  // camera info subscription
  camera_info_sub_ = node_->create_subscription<kratos_msgs::msg::CameraStreamList>(
      "/kratos/available_cameras",
      10,
      [this](kratos_msgs::msg::CameraStreamList::SharedPtr msg) {
        cachedCameras_.clear();
        for (const auto &cam : msg->camera_streams) {
          cachedCameras_.append({
              QString::fromStdString(cam.camera_name), cam.port});
        }
        // immediately emit updated list whenever the topic publishes
        emit availableCamerasUpdated(cachedCameras_);
      });
  RCLCPP_INFO(node_->get_logger(), "Subscribed to /kratos/available_cameras");

  // Create service client
  start_all_client_ = node_->create_client<kratos_msgs::srv::StartAllCameras>(
      "/kratos/cameras/start_all");
}

RosWorker::~RosWorker() {
  // When the worker is destroyed, shut down ROS gracefully
  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

void RosWorker::requestAvailableCameras() {
  // simply resend the last cached list (may be empty if none seen yet)
  emit availableCamerasUpdated(cachedCameras_);
}

void RosWorker::run() {
  // Spin until rclcpp is shut down (blocking call — runs on worker thread)
  rclcpp::spin(node_);
}

void RosWorker::callStartAllCameras() {
  if (!start_all_client_->wait_for_service(std::chrono::seconds(1))) {
    emit serviceResponseReceived(
        false, "Service /kratos/cameras/start_all not available");
    return;
  }

  auto request = std::make_shared<kratos_msgs::srv::StartAllCameras::Request>();

  start_all_client_->async_send_request(
      request,
      [this](rclcpp::Client<kratos_msgs::srv::StartAllCameras>::SharedFuture
                 future) {
        try {
          auto response = future.get();
          emit serviceResponseReceived(
              response->success, QString::fromStdString(response->message));
        } catch (const std::exception &e) {
          emit serviceResponseReceived(
              false, QString("Service call failed: %1").arg(e.what()));
        }
      });
}
