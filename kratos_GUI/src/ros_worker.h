#ifndef ROS_WORKER_H
#define ROS_WORKER_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QPair>
#include <kratos_msgs/srv/start_all_cameras.hpp>
#include <kratos_msgs/msg/camera_stream_list.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp> // existing subscription for /foo


// ─────────────────────────────────────────────────────────────
// RosWorker
// ─────────────────────────────────────────────────────────────
// Runs rclcpp::spin() on a background QThread so the GUI
// stays responsive.  Every time a message arrives on "/foo"
// it emits a Qt signal that the main window can connect to.
// ─────────────────────────────────────────────────────────────
class RosWorker : public QObject {
  Q_OBJECT

public:
  explicit RosWorker(QObject *parent = nullptr);
  ~RosWorker() override;

public slots:
  /// Call this from the worker thread to start spinning.
  void run();

  /// Call the /kratos/cameras/start_all service
  void callStartAllCameras();

  /// Emit the most recently cached list of cameras.  Allows the main window
  /// to request an update on-demand (e.g. when user presses a button).
  void requestAvailableCameras();

signals:
  /// Emitted every time a new std_msgs/String arrives on /foo.
  void messageReceived(const QString &text, double latencyMs);

  /// Emitted when the service call returns
  void serviceResponseReceived(bool success, const QString &message);

  /// Broadcast whenever the camera info topic updates (name,port pairs)
  void availableCamerasUpdated(const QList<QPair<QString,int>> &cameras);

private:
  rclcpp::Node::SharedPtr node_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;

  // camera info subscription
  rclcpp::Subscription<kratos_msgs::msg::CameraStreamList>::SharedPtr
      camera_info_sub_;
  // cache most recent list so we can respond to requests
  QList<QPair<QString,int>> cachedCameras_;

  rclcpp::Client<kratos_msgs::srv::StartAllCameras>::SharedPtr
      start_all_client_;
};

#endif // ROS_WORKER_H
