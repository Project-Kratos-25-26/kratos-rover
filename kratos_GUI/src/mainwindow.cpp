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
  findCameras();
  startRosWorker();
  startVideoWorker();

  connect(ui->cameraSelector,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::onCameraChanged);

  connect(ui->startAllCamerasBtn, &QPushButton::clicked, this,
          &MainWindow::onStartAllCamerasClicked);

  connect(ui->refreshCamerasBtn, &QPushButton::clicked, this,
          &MainWindow::onRefreshCamerasClicked);

}

MainWindow::~MainWindow() {
  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }

  if (rosThread_) {
    rosThread_->quit();
    rosThread_->wait();
  }

  if (videoThread_) {
    videoThread_->quit();
    videoThread_->wait();
  }

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
  appendToLog(QString("Camera selector changed: %1").arg(data));
  if (data.isEmpty()) {
    videoWorker_->stopPipeline();
    ui->videoDisplay->setText("Select a camera to start feed");
    return;
  }

  // data may be a device path ("/dev/video0") or a port number encoded as
  // string.
  bool ok;
  int port = data.toInt(&ok);
  if (ok && port > 0) {
    // pass port string so VideoWorker knows to build tcp pipeline
    QMetaObject::invokeMethod(videoWorker_, "startPipeline",
                              Qt::QueuedConnection, Q_ARG(QString, QString::number(port)));
  } else {
    QMetaObject::invokeMethod(videoWorker_, "startPipeline",
                              Qt::QueuedConnection, Q_ARG(QString, data));
  }
}

void MainWindow::onStartAllCamerasClicked() {
  ui->startAllCamerasBtn->setEnabled(false);
  ui->statusLbl_->setText("Calling service /kratos/cameras/start_all...");
  QMetaObject::invokeMethod(rosWorker_, "callStartAllCameras",
                            Qt::QueuedConnection);
}

void MainWindow::onServiceResponse(bool success, const QString &message) {
  ui->startAllCamerasBtn->setEnabled(true);
  ui->statusLbl_->setText(QString("Service response: %1").arg(message));
  QString line = QString("[SERVICE] %1: %2")
                     .arg(success ? "SUCCESS" : "ERROR")
                     .arg(message);
  ui->logOutput_->append(line);
  appendToLog(line);
}

void MainWindow::onRefreshCamerasClicked() {
  appendToLog("User requested camera list refresh");
  if (rosWorker_) {
    ui->statusLbl_->setText("Requesting available camera list...");
    QMetaObject::invokeMethod(rosWorker_, "requestAvailableCameras",
                              Qt::QueuedConnection);
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
  appendToLog(QString("Cameras: %1").arg(entries.join(", ")));
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
    logStream_ << text << "\n";
    logStream_.flush();
  }
}
