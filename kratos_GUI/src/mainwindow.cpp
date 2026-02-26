#include "mainwindow.h"
#include "ros_worker.h"
#include "ui_mainwindow.h"
#include "video_worker.h"

#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // make sure our signal type is known to Qt's meta-object system since it
  // crosses threads
  qRegisterMetaType<QPair<QString,int>>("QPair<QString,int>");
  qRegisterMetaType<QList<QPair<QString,int>>>("QList<QPair<QString,int>>");

  openLogFile();
  appendToLog("[GUI_STARTUP] MainWindow constructor started");
  appendToLog("[GUI_STARTUP] NOTE: No automatic service calls. User must click 'Start All Cameras'.");
  
  findCameras();
  appendToLog("[GUI_STARTUP] Local /dev/video* devices enumerated");
  
  startRosWorker();
  appendToLog("[GUI_STARTUP] ROS worker thread started");
  
  startVideoWorker();
  appendToLog("[GUI_STARTUP] Video worker thread started");

  connect(ui->cameraSelector,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::onCameraChanged);

  connect(ui->startAllCamerasBtn, &QPushButton::clicked, this,
          &MainWindow::onStartAllCamerasClicked);

  connect(ui->refreshCamerasBtn, &QPushButton::clicked, this,
          &MainWindow::onRefreshCamerasClicked);

  appendToLog("[GUI_STARTUP] All UI connections established. MainWindow ready.");
}

MainWindow::~MainWindow() {
  appendToLog("[GUI_SHUTDOWN] MainWindow destructor called");
  
  if (rclcpp::ok()) {
    appendToLog("[GUI_SHUTDOWN] ROS still running, calling rclcpp::shutdown()");
    rclcpp::shutdown();
  }

  if (rosThread_) {
    appendToLog("[GUI_SHUTDOWN] Waiting for ROS worker thread to finish...");
    rosThread_->quit();
    rosThread_->wait();
    appendToLog("[GUI_SHUTDOWN] ROS worker thread finished");
  }

  if (videoThread_) {
    appendToLog("[GUI_SHUTDOWN] Waiting for video worker thread to finish...");
    videoThread_->quit();
    videoThread_->wait();
    appendToLog("[GUI_SHUTDOWN] Video worker thread finished");
  }

  appendToLog("[GUI_SHUTDOWN] MainWindow cleanup complete. Application exiting.");
  logFile_.close();
  delete ui;
}

void MainWindow::findCameras() {
  // initially populate with /dev entries so the user still can view local
  // feeds; this will be overwritten once /kratos/available_cameras is
  // received
  ui->cameraSelector->clear();
  ui->cameraSelector->addItem("Select Camera...", "");

  QDir devDir("/dev");
  QStringList cameras = devDir.entryList({"video*"}, QDir::System);
  for (const QString &cam : cameras) {
    ui->cameraSelector->addItem(cam, "/dev/" + cam);
  }
}

void MainWindow::startRosWorker() {
  rosThread_ = new QThread(this);
  rosWorker_ = new RosWorker();
  rosWorker_->moveToThread(rosThread_);
  connect(rosThread_, &QThread::started, rosWorker_, &RosWorker::run);
  connect(rosWorker_, &RosWorker::messageReceived, this,
          &MainWindow::onRosMessage, Qt::QueuedConnection);
  connect(rosWorker_, &RosWorker::serviceResponseReceived, this,
          &MainWindow::onServiceResponse, Qt::QueuedConnection);
  connect(rosWorker_, &RosWorker::availableCamerasUpdated, this,
          &MainWindow::onAvailableCamerasUpdated, Qt::QueuedConnection);
  connect(rosThread_, &QThread::finished, rosWorker_, &QObject::deleteLater);
  rosThread_->start();
}

void MainWindow::startVideoWorker() {
  videoThread_ = new QThread(this);
  videoWorker_ = new VideoWorker();
  videoWorker_->moveToThread(videoThread_);

  connect(videoWorker_, &VideoWorker::newFrame, this,
          &MainWindow::onNewVideoFrame, Qt::QueuedConnection);

  connect(videoThread_, &QThread::finished, videoWorker_,
          &QObject::deleteLater);
  videoThread_->start();
}

void MainWindow::onRosMessage(const QString &text, double latencyMs) {
  ++msgCount_;
  QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  QString line = QString("[%1] [%2] %3 (Latency: %4 ms)")
                     .arg(timestamp)
                     .arg(msgCount_)
                     .arg(text)
                     .arg(latencyMs, 0, 'f', 2);
  ui->logOutput_->append(line);
  ui->statusLbl_->setText(QString("Messages received: %1 | Latency: %2 ms")
                              .arg(msgCount_)
                              .arg(latencyMs, 0, 'f', 2));
  appendToLog(line);
}

void MainWindow::onNewVideoFrame(const QImage &frame) {
  ui->videoDisplay->setPixmap(QPixmap::fromImage(
      frame.scaled(ui->videoDisplay->size(), Qt::KeepAspectRatio,
                   Qt::SmoothTransformation)));
}

void MainWindow::onCameraChanged(int index) {
  QString data = ui->cameraSelector->itemData(index).toString();
  QString displayName = ui->cameraSelector->itemText(index);
  
  if (data.isEmpty()) {
    QString msg = QString("[CAMERA_SELECTION] User selected: %1 (no stream started)").arg(displayName);
    appendToLog(msg);
    videoWorker_->stopPipeline();
    ui->videoDisplay->setText("Select a camera to start feed");
    return;
  }

  // data may be a device path ("/dev/video0") or a port number encoded as
  // string.
  bool ok;
  int port = data.toInt(&ok);
  if (ok && port > 0) {
    // TCP stream from kratos_cameras
    QString logMsg = QString("[CAMERA_SELECTION] Starting TCP stream: %1 (port %2)").arg(displayName).arg(port);
    appendToLog(logMsg);
    ui->statusLbl_->setText(logMsg);
    QMetaObject::invokeMethod(videoWorker_, "startPipeline",
                              Qt::QueuedConnection, Q_ARG(QString, QString::number(port)));
  } else {
    // Local v4l2 device
    QString logMsg = QString("[CAMERA_SELECTION] Starting local device: %1 (%2)").arg(displayName).arg(data);
    appendToLog(logMsg);
    ui->statusLbl_->setText(logMsg);
    QMetaObject::invokeMethod(videoWorker_, "startPipeline",
                              Qt::QueuedConnection, Q_ARG(QString, data));
  }
}

void MainWindow::onStartAllCamerasClicked() {
  ui->startAllCamerasBtn->setEnabled(false);
  QString msg = "[USER_ACTION] 'Start All Cameras' button clicked - calling service";
  ui->statusLbl_->setText(msg);
  appendToLog(msg);
  QMetaObject::invokeMethod(rosWorker_, "callStartAllCameras",
                            Qt::QueuedConnection);
}

void MainWindow::onServiceResponse(bool success, const QString &message) {
  ui->startAllCamerasBtn->setEnabled(true);
  ui->statusLbl_->setText(QString("Service response: %1").arg(message));
  QString line = QString("[SERVICE_RESPONSE] %1: %2")
                     .arg(success ? "SUCCESS" : "FAILURE")
                     .arg(message);
  ui->logOutput_->append(line);
  appendToLog(line);
}

void MainWindow::onRefreshCamerasClicked() {
  QString msg = "[USER_ACTION] 'Refresh Cameras' button clicked - requesting camera list";
  appendToLog(msg);
  if (rosWorker_) {
    ui->statusLbl_->setText(msg);
    QMetaObject::invokeMethod(rosWorker_, "requestAvailableCameras",
                              Qt::QueuedConnection);
  } else {
    appendToLog("[ERROR] rosWorker is null!");
  }
}

void MainWindow::onAvailableCamerasUpdated(const QList<QPair<QString,int>> &cameras) {
  ui->cameraSelector->blockSignals(true);
  ui->cameraSelector->clear();
  ui->cameraSelector->addItem("Select Camera...", "");
  QStringList entries;
  for (const auto &pair : cameras) {
    QString name = pair.first;
    int port = pair.second;
    QString display = QString("%1 (%2)").arg(name).arg(port);
    ui->cameraSelector->addItem(display, QString::number(port));
    entries << QString("%1:%2").arg(name).arg(port);
  }
  ui->cameraSelector->blockSignals(false);
  ui->statusLbl_->setText("Camera list updated");
  QString logMsg = QString("[CAMERA_UPDATE] Refreshed camera list: %1 camera(s) available - [%2]")
                      .arg(cameras.size())
                      .arg(entries.join(", "));
  appendToLog(logMsg);
}

void MainWindow::openLogFile() {
  logFile_.setFileName("kratos_gui.log");
  if (logFile_.open(QIODevice::Append | QIODevice::Text)) {
    logStream_.setDevice(&logFile_);
    appendToLog("=== application started ===");
  }
}

void MainWindow::appendToLog(const QString &text) {
  if (logStream_.device()) {
    QString timestamp = QDateTime::currentDateTime().toString("[yyyy-MM-dd HH:mm:ss.zzz]");
    logStream_ << timestamp << " " << text << "\n";
    logStream_.flush();
  }
}
