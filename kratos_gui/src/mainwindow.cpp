#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QPixmap>
#include <QPushButton>

static bool _registered = []() {
  qRegisterMetaType<CameraStatusList>("CameraStatusList");
  qRegisterMetaType<QImage>("QImage");
  return true;
}();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Styling for the main window overall.
  setStyleSheet("QMainWindow { background-color: #16213e; }"
                "QScrollArea { border: none; background: transparent; }"
                "QWidget#scrollAreaWidgetContents { background: transparent; }"
                "QStatusBar { background: #1a1a2e; color: #888; }");

  // Keep a pointer to the layout
  cameraButtonsLayout_ =
      qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());

  // Set up ROS Worker
  rosWorker_ = new RosWorker();
  rosWorker_->moveToThread(&rosThread_);
  connect(&rosThread_, &QThread::started, rosWorker_, &RosWorker::init);
  connect(&rosThread_, &QThread::finished, rosWorker_, &QObject::deleteLater);

  // Set up Video Worker
  videoWorker_ = new VideoWorker();
  videoWorker_->moveToThread(&videoThread_);
  connect(&videoThread_, &QThread::finished, videoWorker_,
          &QObject::deleteLater);

  // Connect ROS signals
  connect(rosWorker_, &RosWorker::camerasUpdated, this,
          &MainWindow::onCamerasUpdated);
  connect(rosWorker_, &RosWorker::serviceResult, this,
          &MainWindow::onServiceResult);

  // Connect Video signals
  connect(videoWorker_, &VideoWorker::newFrame, this, &MainWindow::onNewFrame,
          Qt::QueuedConnection);
  connect(videoWorker_, &VideoWorker::pipelineError, this,
          &MainWindow::onPipelineError, Qt::QueuedConnection);

  rosThread_.start();
  videoThread_.start();

  ui->statusbar->showMessage("Waiting for camera streams...");
}

MainWindow::~MainWindow() {
  QMetaObject::invokeMethod(videoWorker_, "stopPipeline", Qt::QueuedConnection);

  rosThread_.quit();
  videoThread_.quit();

  rosThread_.wait();
  videoThread_.wait();

  delete ui;
}

void MainWindow::onCamerasUpdated(CameraStatusList cameras) {
  // 1. Remove all existing buttons (keep the spacer at the end)
  QLayoutItem *child;
  for (int i = cameraButtonsLayout_->count() - 1; i >= 0; --i) {
    child = cameraButtonsLayout_->itemAt(i);
    if (child->widget()) {
      delete child->widget();
    }
  }

  // 2. Add buttons and check current viewed camera status
  if (cameras.isEmpty()) {
    ui->statusbar->showMessage("No cameras available.");
    QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                              Qt::QueuedConnection);
    ui->videoLabel->setText("No video feed");
    currentViewedCamera_.clear();
    currentCameraIsActive_ = false;
  } else {
    bool foundCurrentViewed = false;

    for (const auto &camInfo : cameras) {
      QString name = camInfo.name;
      bool isActive = camInfo.active;
      int port = camInfo.port;

      QPushButton *btn = new QPushButton(name, ui->scrollAreaWidgetContents);

      btn->setProperty("cameraName", name);
      btn->setProperty("isActive", isActive);

      QString btnStyle =
          "QPushButton { "
          "  color: #e0e0e0; padding: 12px; border: none; border-radius: 6px; "
          "font-weight: bold; font-size: 16px; "
          "  background-color: %1; "
          "}"
          "QPushButton:hover { background-color: %2; }"
          "QPushButton:pressed { background-color: %3; }";

      if (isActive) {
        btn->setStyleSheet(
            btnStyle.arg("#c73e52").arg("#e94560").arg("#a12e3e"));
        btn->setText(name + " (Running — Click to Stop)");
      } else {
        btn->setStyleSheet(
            btnStyle.arg("#0f3460").arg("#1a4a80").arg("#0a2545"));
        btn->setText(name + " (Stopped — Click to Start)");
      }

      connect(btn, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
      cameraButtonsLayout_->insertWidget(cameraButtonsLayout_->count() - 1,
                                         btn);

      // Auto-start video if the camera we clicked on just became active
      if (name == currentViewedCamera_) {
        foundCurrentViewed = true;

        // State transition: Inactive -> Active = Start pipeline
        if (isActive && !currentCameraIsActive_) {
          ui->videoLabel->setText(QString("Connecting to %1...").arg(name));
          QMetaObject::invokeMethod(
              videoWorker_, "startPipeline", Qt::QueuedConnection,
              Q_ARG(QString, "192.168.1.10"), Q_ARG(int, port));
        }
        // State transition: Active -> Inactive = Stop pipeline
        else if (!isActive && currentCameraIsActive_) {
          QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                                    Qt::QueuedConnection);
          ui->videoLabel->setText("Video Stopped");
        }
        currentCameraIsActive_ = isActive;
      }
    }

    // If the camera we were viewing disappeared from the topic completely
    if (!foundCurrentViewed && !currentViewedCamera_.isEmpty()) {
      QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                                Qt::QueuedConnection);
      ui->videoLabel->setText("Camera Disconnected");
      currentViewedCamera_.clear();
      currentCameraIsActive_ = false;
    }

    // ui->statusbar->showMessage(QString("Found %1
    // camera(s).").arg(cameras.size()));
  }
}

void MainWindow::onButtonClicked() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;

  QString name = btn->property("cameraName").toString();
  bool isActive = btn->property("isActive").toBool();

  // Set as the current camera we are interested in viewing
  currentViewedCamera_ = name;
  currentCameraIsActive_ = isActive;

  if (isActive) {
    ui->statusbar->showMessage(QString("Stopping stream for %1...").arg(name));
    QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  } else {
    ui->statusbar->showMessage(QString("Starting stream for %1...").arg(name));
    QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  }
}

void MainWindow::onServiceResult(bool success, QString message) {
  ui->statusbar->showMessage(
      (success ? QString("✓ ") : QString("✗ ")) + message, 5000);
}

void MainWindow::onNewFrame(QImage frame) {
  ui->videoLabel->setPixmap(QPixmap::fromImage(frame).scaled(
      ui->videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::onPipelineError(QString errorMsg) {
  ui->statusbar->showMessage("Video error: " + errorMsg, 5000);
  ui->videoLabel->setText("Video error — see status bar");
}
