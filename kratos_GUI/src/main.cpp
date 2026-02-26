#include "mainwindow.h"

#include <QApplication>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char *argv[]) {
  // 1. Init ROS 2 first (parses ROS args)
  rclcpp::init(argc, argv);

  // 2. Init Qt
  QApplication app(argc, argv);

  // 3. Show the window
  MainWindow window;
  window.show();

  // 4. Run the Qt event loop (ROS spins in a background thread)
  int ret = app.exec();

  // 5. Cleanup
  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }

  return ret;
}
