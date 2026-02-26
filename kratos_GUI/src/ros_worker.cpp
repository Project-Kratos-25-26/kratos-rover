#include "ros_worker.h"

#include <chrono>

RosWorker::RosWorker(QObject *parent) : QObject(parent) {
  // register camera list type so we can emit it across threads
  qRegisterMetaType<QPair<QString,int>>("QPair<QString,int>");
  qRegisterMetaType<QList<QPair<QString,int>>>("QList<QPair<QString,int>>");

  // Create a ROS 2 node called "gui_vibecoded_node"
  node_ = rclcpp::Node::make_shared("gui_vibecoded_node");
  RCLCPP_INFO(node_->get_logger(), "[GUI] ROS node initialized: gui_vibecoded_node");

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

  RCLCPP_INFO(node_->get_logger(), "[GUI] Successfully subscribed to /foo");

  // camera info subscription
  camera_info_sub_ = node_->create_subscription<kratos_msgs::msg::CameraStreamList>(
      "/kratos/available_cameras",
      10,
      [this](kratos_msgs::msg::CameraStreamList::SharedPtr msg) {
        cachedCameras_.clear();
        std::string camList;
        for (const auto &cam : msg->camera_streams) {
          cachedCameras_.append({
              QString::fromStdString(cam.camera_name), cam.port});
          camList += std::string(cam.camera_name) + ":" + std::to_string(cam.port) + "; ";
        }
        RCLCPP_INFO(node_->get_logger(), 
            "[CAMERA_DISCOVERY] Received camera list: [%s] (count: %zu)",
            camList.c_str(), msg->camera_streams.size());
        // immediately emit updated list whenever the topic publishes
        emit availableCamerasUpdated(cachedCameras_);
      });
  RCLCPP_INFO(node_->get_logger(), "[GUI] Successfully subscribed to /kratos/available_cameras");

  // Create service client
  start_all_client_ = node_->create_client<kratos_msgs::srv::StartAllCameras>(
      "/kratos/cameras/start_all");
  RCLCPP_INFO(node_->get_logger(), "[GUI] Service client created for /kratos/cameras/start_all");
  RCLCPP_INFO(node_->get_logger(), "[GUI] INFO: No automatic service calls on startup. User must click 'Start All Cameras' button.");
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
  RCLCPP_INFO(node_->get_logger(), "[GUI:SPIN] Starting ROS spin loop...");
  rclcpp::spin(node_);
  RCLCPP_INFO(node_->get_logger(), "[GUI:SPIN] ROS spin loop ended.");
}

void RosWorker::callStartAllCameras() {
  RCLCPP_INFO(node_->get_logger(), "[SERVICE_CALL] Attempting to call /kratos/cameras/start_all");
  
  if (!start_all_client_->wait_for_service(std::chrono::seconds(1))) {
    RCLCPP_ERROR(node_->get_logger(), 
        "[SERVICE_CALL_ERROR] Service /kratos/cameras/start_all not available within timeout");
    emit serviceResponseReceived(
        false, "Service /kratos/cameras/start_all not available");
    return;
  }

  RCLCPP_INFO(node_->get_logger(), "[SERVICE_CALL] Service found. Sending request...");
  auto request = std::make_shared<kratos_msgs::srv::StartAllCameras::Request>();

  start_all_client_->async_send_request(
      request,
      [this](rclcpp::Client<kratos_msgs::srv::StartAllCameras>::SharedFuture
                 future) {
        try {
          auto response = future.get();
          if (response->success) {
            RCLCPP_INFO(node_->get_logger(), 
                "[SERVICE_RESPONSE] StartAllCameras SUCCESS: %s",
                response->message.c_str());
          } else {
            RCLCPP_WARN(node_->get_logger(), 
                "[SERVICE_RESPONSE] StartAllCameras FAILED: %s",
                response->message.c_str());
          }
          emit serviceResponseReceived(
              response->success, QString::fromStdString(response->message));
        } catch (const std::exception &e) {
          RCLCPP_ERROR(node_->get_logger(), 
              "[SERVICE_CALL_ERROR] Exception during async service call: %s",
              e.what());
          emit serviceResponseReceived(
              false, QString("Service call failed: %1").arg(e.what()));
        }
      });
}
