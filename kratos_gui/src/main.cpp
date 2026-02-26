#include "mainwindow.h"
#include <QApplication>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char *argv[]) {
  // Initialize ROS2
  rclcpp::init(argc, argv);

  // Initialize Qt
  QApplication a(argc, argv);

  // Create and show the main window
  MainWindow w;
  w.show();

  // Run the Qt event loop
  int ret = a.exec();

  // Cleanup ROS2
  rclcpp::shutdown();

  return ret;
}
