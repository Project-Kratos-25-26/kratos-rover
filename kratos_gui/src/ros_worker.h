#ifndef ROS_WORKER_H
#define ROS_WORKER_H

#include <QImage>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include <kratos_msgs/msg/camera_stream_list.hpp>
#include <kratos_msgs/srv/start_camera_stream.hpp>
#include <kratos_msgs/srv/stop_camera_stream.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/joy.hpp>

// Struct to store camera info
struct CameraInfo {
  QString name;
  bool active;
  int port;
};

using CameraStatusList = QList<CameraInfo>;

class RosWorker : public QObject {
  Q_OBJECT
public:
  explicit RosWorker(QObject *parent = nullptr);
  ~RosWorker() override;

signals:
  void camerasUpdated(CameraStatusList cameras);
  void serviceResult(bool success, QString message);
  void joystickDataUpdated(QList<float> axes, QList<int> buttons);
  void zedImageReceived(QImage frame);

public slots:
  void init();
  void callStartStream(QString cameraName);
  void callStopStream(QString cameraName);
  void subscribeToZed();
  void unsubscribeFromZed();

private:
  void
  onCameraListReceived(const kratos_msgs::msg::CameraStreamList::SharedPtr msg);
  void onJoyReceived(const sensor_msgs::msg::Joy::SharedPtr msg);
  void onZedImageReceived(const sensor_msgs::msg::Image::SharedPtr msg);

  rclcpp::Node::SharedPtr node_;
  QTimer *spinTimer_ = nullptr;

  rclcpp::Subscription<kratos_msgs::msg::CameraStreamList>::SharedPtr
      cameraSub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joySub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr zedSub_;
  int zedSubscriptionCount_ = 0;

  rclcpp::Client<kratos_msgs::srv::StartCameraStream>::SharedPtr
      startStreamClient_;
  rclcpp::Client<kratos_msgs::srv::StopCameraStream>::SharedPtr
      stopStreamClient_;
};

// Declare to Qt meta type system
Q_DECLARE_METATYPE(CameraInfo)

#endif // ROS_WORKER_H
