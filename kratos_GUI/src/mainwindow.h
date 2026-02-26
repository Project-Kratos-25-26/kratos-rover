#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QTabWidget>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QPair>
#include <QThread>

namespace Ui {
class MainWindow;
}

class RosWorker; // forward declaration
class VideoWorker;

// ─────────────────────────────────────────────────────────────
// MainWindow
// ─────────────────────────────────────────────────────────────
// Three tabs:
//   1. View ROS Topic  — live log of messages from /foo
//   2. Configurations  — placeholder (TODO)
//   3. Cams            — placeholder (TODO)
// ─────────────────────────────────────────────────────────────
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private slots:
  /// Connected to RosWorker::messageReceived
  void onRosMessage(const QString &text, double latencyMs);

  /// Connected to VideoWorker::newFrame
  void onNewVideoFrame(const QImage &frame);

  /// Triggered when the user selects a different camera source (port or device)
  void onCameraChanged(int index);

  /// Triggered when the "Start All Cameras" button is clicked
  void onStartAllCamerasClicked();

  /// Triggered when a service response is received from RosWorker
  void onServiceResponse(bool success, const QString &message);

  /// Triggered when the user clicks "Refresh Cameras" button
  void onRefreshCamerasClicked();

  /// Invoked whenever RosWorker publishes an updated camera list
  void onAvailableCamerasUpdated(const QList<QPair<QString,int>> &cameras);

private:
  void startRosWorker();
  void startVideoWorker();
  void findCameras();
  void openLogFile();
  void appendToLog(const QString &text);

  Ui::MainWindow *ui;
  int msgCount_ = 0;

  // filesystem logging
  QFile logFile_;
  QTextStream logStream_;

  // ── ROS background thread ──
  QThread *rosThread_ = nullptr;
  RosWorker *rosWorker_ = nullptr;

  // ── Video background thread ──
  QThread *videoThread_ = nullptr;
  VideoWorker *videoWorker_ = nullptr;
};

#endif // MAINWINDOW_H
