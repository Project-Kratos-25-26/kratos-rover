#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QVBoxLayout>

#include "ros_worker.h"
#include "video_worker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  void onCamerasUpdated(CameraStatusList cameras);
  void onButtonClicked();
  void onServiceResult(bool success, QString message);
  void onNewFrame(QImage frame);
  void onPipelineError(QString errorMsg);

private:
  Ui::MainWindow *ui;

  QThread rosThread_;
  RosWorker *rosWorker_ = nullptr;

  QThread videoThread_;
  VideoWorker *videoWorker_ = nullptr;

  // Layout where the buttons will be added dynamically
  QVBoxLayout *cameraButtonsLayout_;

  // Track the currently viewed camera so we know what port to connect to
  QString currentViewedCamera_;
  bool currentCameraIsActive_ = false;
};

#endif // MAINWINDOW_H
