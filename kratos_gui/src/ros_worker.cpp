#include "ros_worker.h"
#include <QDebug>

RosWorker::RosWorker(QObject *parent) : QObject(parent) {}

RosWorker::~RosWorker() {
  if (spinTimer_) {
    spinTimer_->stop();
  }
  cameraSub_.reset();
  joySub_.reset();
  zedSub_.reset();
  startStreamClient_.reset();
  stopStreamClient_.reset();
  node_.reset();
}

void RosWorker::init() {
  node_ = rclcpp::Node::make_shared("kratos_gui_node");

  cameraSub_ = node_->create_subscription<kratos_msgs::msg::CameraStreamList>(
      "/kratos/available_cameras", 10,
      std::bind(&RosWorker::onCameraListReceived, this, std::placeholders::_1));

  joySub_ = node_->create_subscription<sensor_msgs::msg::Joy>(
      "/joy", 10,
      std::bind(&RosWorker::onJoyReceived, this, std::placeholders::_1));

  startStreamClient_ =
      node_->create_client<kratos_msgs::srv::StartCameraStream>(
          "/kratos/cameras/start_stream");
  stopStreamClient_ = node_->create_client<kratos_msgs::srv::StopCameraStream>(
      "/kratos/cameras/stop_stream");

  // Spin ROS node without blocking Qt event loop
  spinTimer_ = new QTimer(this);
  connect(spinTimer_, &QTimer::timeout, this, [this]() {
    if (rclcpp::ok()) {
      rclcpp::spin_some(node_);
    }
  });
  spinTimer_->start(10); // 10ms ~ 100Hz
}

void RosWorker::onCameraListReceived(
    const kratos_msgs::msg::CameraStreamList::SharedPtr msg) {
  CameraStatusList cameras;
  for (const auto &cam : msg->camera_streams) {
    CameraInfo info;
    info.name = QString::fromStdString(cam.camera_name);
    info.active = cam.active;
    info.port = cam.port;
    cameras.append(info);
  }

  // Emit the new list of camera statuses
  emit camerasUpdated(cameras);
}

void RosWorker::onJoyReceived(const sensor_msgs::msg::Joy::SharedPtr msg) {
  QList<float> axes(msg->axes.begin(), msg->axes.end());
  QList<int> buttons(msg->buttons.begin(), msg->buttons.end());
  emit joystickDataUpdated(axes, buttons);
}

void RosWorker::callStartStream(QString cameraName) {
  if (!startStreamClient_->wait_for_service(std::chrono::seconds(2))) {
    emit serviceResult(false, "start_stream service not available");
    return;
  }
  auto req = std::make_shared<kratos_msgs::srv::StartCameraStream::Request>();
  req->camera_name = cameraName.toStdString();

  startStreamClient_->async_send_request(
      req,
      [this](
          rclcpp::Client<kratos_msgs::srv::StartCameraStream>::SharedFuture f) {
        auto res = f.get();
        emit serviceResult(res->success, QString::fromStdString(res->message));
      });
}

void RosWorker::callStopStream(QString cameraName) {
  if (!stopStreamClient_->wait_for_service(std::chrono::seconds(2))) {
    emit serviceResult(false, "stop_stream service not available");
    return;
  }
  auto req = std::make_shared<kratos_msgs::srv::StopCameraStream::Request>();
  req->camera_name = cameraName.toStdString();

  stopStreamClient_->async_send_request(
      req,
      [this](
          rclcpp::Client<kratos_msgs::srv::StopCameraStream>::SharedFuture f) {
        auto res = f.get();
        emit serviceResult(res->success, QString::fromStdString(res->message));
      });
}

void RosWorker::subscribeToZed() {
  zedSubscriptionCount_++;
  qDebug() << "subscribeToZed() called. Count:" << zedSubscriptionCount_;
  if (zedSubscriptionCount_ == 1) {
    qDebug() << "Subscribing to ZED topic /zed/zed_node/rgb/color/rect/image";
    // Only subscribe the first time someone needs ZED
    zedSub_ = node_->create_subscription<sensor_msgs::msg::Image>(
        "/zed/zed_node/rgb/color/rect/image", 10,
        std::bind(&RosWorker::onZedImageReceived, this, std::placeholders::_1));
  }
}

void RosWorker::unsubscribeFromZed() {
  if (zedSubscriptionCount_ > 0) {
    zedSubscriptionCount_--;
    qDebug() << "unsubscribeFromZed() called. Count:" << zedSubscriptionCount_;
    if (zedSubscriptionCount_ == 0) {
      qDebug() << "Unsubscribing from ZED topic";
      // Unsubscribe when no one needs ZED anymore
      zedSub_.reset();
    }
  }
}

void RosWorker::onZedImageReceived(
    const sensor_msgs::msg::Image::SharedPtr msg) {
  QImage::Format format = QImage::Format_Invalid;

  if (msg->encoding == "rgb8") {
    format = QImage::Format_RGB888;
  } else if (msg->encoding == "bgr8") {
    format = QImage::Format_BGR888;
  } else if (msg->encoding == "rgba8") {
    format = QImage::Format_RGBA8888;
  } else if (msg->encoding == "bgra8") {
    // ROS uses BGRA for ZED default
    format = QImage::Format_ARGB32; // In Qt, ARGB32 is physically BGRA in
                                    // memory on little endian
  } else {
    // Attempt fallback or emit error. For safety we just ignore unsupported
    // formats.
    return;
  }

  // Create QImage from data. Deep copy is required.
  QImage frame(msg->data.data(), msg->width, msg->height, msg->step, format);
  emit zedImageReceived(frame.copy());
}
