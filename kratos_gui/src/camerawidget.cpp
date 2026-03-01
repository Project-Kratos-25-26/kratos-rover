#include "camerawidget.h"
#include "ui_camerawidget.h"

#include <QDebug>
#include <QPushButton>
#include <QStyle>

#include "videoplayerwidget.h"

CameraWidget::CameraWidget(RosWorker *rosWorker, QWidget *parent)
    : QWidget(parent), ui(new Ui::CameraWidget), rosWorker_(rosWorker) {
  ui->setupUi(this);

  cameraButtonsLayout_ =
      qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());

  // Dark Isaac Sim Aesthetic for modules
  ui->scrollArea->setStyleSheet(
      "QScrollArea { border: none; background-color: #1C1C1C; }");
  ui->scrollAreaWidgetContents->setStyleSheet(
      "QWidget { background-color: #1C1C1C; }");

  // Connect ROS signals (shared worker)
  connect(rosWorker_, &RosWorker::camerasUpdated, this,
          &CameraWidget::onCamerasUpdated);
}

CameraWidget::~CameraWidget() {
  // Stop all active streams
  for (auto wrapper : activePlayers_) {
    VideoPlayerWidget *player =
        qobject_cast<VideoPlayerWidget *>(wrapper->getChildWidget());
    if (player) {
      player->stopStream();
    }
  }

  delete ui;
}

void CameraWidget::onCamerasUpdated(CameraStatusList cameras) {
  QLayoutItem *child;
  for (int i = cameraButtonsLayout_->count() - 1; i >= 0; --i) {
    child = cameraButtonsLayout_->itemAt(i);
    if (child->widget()) {
      delete child->widget();
    }
  }

  if (cameras.isEmpty()) {
    if (currentViewedCamera_ == "ZED" && currentCameraIsActive_) {
      QMetaObject::invokeMethod(rosWorker_, "unsubscribeFromZed",
                                Qt::QueuedConnection);
      disconnect(rosWorker_, &RosWorker::zedImageReceived, this,
                 &CameraWidget::onNewFrame);
    } else {
      QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                                Qt::QueuedConnection);
    }

    ui->videoLabel->setText("NO VIDEO FEED");
    ui->videoLabel->setProperty("active", false);
    ui->videoLabel->style()->unpolish(ui->videoLabel);
    ui->videoLabel->style()->polish(ui->videoLabel);
    currentViewedCamera_.clear();
    currentCameraIsActive_ = false;
  } else {
    bool foundCurrentViewed = false;

    for (const auto &camInfo : cameras) {
      QString name = camInfo.name;
      bool isActive = camInfo.active;
      int port = camInfo.port;

      if (name == "ZED") {
        isActive = isZedLocallySubscribed_;
      }

      QPushButton *btn = new QPushButton(name, ui->scrollAreaWidgetContents);
      btn->setProperty("cameraName", name);
      btn->setProperty("isActive", isActive);

      QString btnStyle = R"(
                QPushButton { 
                    color: #AAAAAA; 
                    padding: 8px 12px; 
                    border: 1px solid #333333; 
                    border-radius: 0px; 
                    font-size: 11px; 
                    background-color: #1C1C1C; 
                    text-align: left;
                }
                QPushButton:hover { 
                    background-color: #252525; 
                    color: #FFFFFF;
                }
            )";

      if (isActive) {
        btn->setStyleSheet(R"(
                    QPushButton { 
                        color: #FFFF55; 
                        padding: 8px 12px; 
                        border: 1px solid #FFFF55; 
                        border-radius: 0px; 
                        font-size: 11px; 
                        background-color: #252525; 
                        text-align: left;
                    }
                    QPushButton:hover { 
                        background-color: #333333; 
                    }
                )");
        btn->setText("■ " + name + " (ACTIVE)");
      } else {
        btn->setStyleSheet(btnStyle);
        btn->setText("▶ " + name + " (OFF)");
      }

      connect(btn, &QPushButton::clicked, this, &CameraWidget::onButtonClicked);
      cameraButtonsLayout_->insertWidget(cameraButtonsLayout_->count() - 1,
                                         btn);

      if (name == currentViewedCamera_) {
        foundCurrentViewed = true;

        if (name != "ZED") {
          if (isActive && !currentCameraIsActive_) {
            ui->videoLabel->setProperty("active", true);
            ui->videoLabel->style()->unpolish(ui->videoLabel);
            ui->videoLabel->style()->polish(ui->videoLabel);
            ui->videoLabel->setText(QString("CONNECTING TO %1...").arg(name));

            QMetaObject::invokeMethod(
                videoWorker_, "startPipeline", Qt::QueuedConnection,
                Q_ARG(QString, "192.168.1.10"), Q_ARG(int, port));
          } else if (!isActive && currentCameraIsActive_) {
            QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                                      Qt::QueuedConnection);
            ui->videoLabel->setProperty("active", false);
            ui->videoLabel->style()->unpolish(ui->videoLabel);
            ui->videoLabel->style()->polish(ui->videoLabel);
            ui->videoLabel->setText("VIDEO STOPPED");
          }
          currentCameraIsActive_ = isActive;
        }
      }
    }

    if (!foundCurrentViewed && !currentViewedCamera_.isEmpty() &&
        currentViewedCamera_ != "ZED") {
      QMetaObject::invokeMethod(videoWorker_, "stopPipeline",
                                Qt::QueuedConnection);
      ui->videoLabel->setProperty("active", false);
      ui->videoLabel->style()->unpolish(ui->videoLabel);
      ui->videoLabel->style()->polish(ui->videoLabel);
      ui->videoLabel->setText("CAMERA DISCONNECTED");
      currentViewedCamera_.clear();
      currentCameraIsActive_ = false;
    }
  }
}

void CameraWidget::onButtonClicked() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;

  QString name = btn->property("cameraName").toString();
  bool isActive = btn->property("isActive").toBool();
  int port = btn->property("port").toInt();

  if (name == "ZED") {
    if (activePlayers_.contains("ZED")) {
      // It's currently active, shut it down
      DraggableWrapper *wrapper = activePlayers_.take("ZED");
      if (wrapper)
        wrapper->deleteLater();
    } else {
      // Spawn new ZED display
      VideoPlayerWidget *player = new VideoPlayerWidget(name, port, rosWorker_);
      DraggableWrapper *wrapper =
          new DraggableWrapper(player, ui->canvasWidget);
      wrapper->resize(640, 480);
      wrapper->move(20, 20); // offset a bit
      wrapper->show();
      activePlayers_.insert("ZED", wrapper);
      player->startStream();
    }
    return;
  }

  if (isActive) {
    if (activePlayers_.contains(name)) {
      DraggableWrapper *wrapper = activePlayers_.take(name);
      if (wrapper)
        wrapper->deleteLater();
    }
    QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  } else {
    if (!activePlayers_.contains(name)) {
      VideoPlayerWidget *player = new VideoPlayerWidget(name, port, rosWorker_);
      DraggableWrapper *wrapper =
          new DraggableWrapper(player, ui->canvasWidget);
      wrapper->resize(640, 480);
      // Slightly cascade windows
      int offset = activePlayers_.count() * 40;
      wrapper->move(20 + offset, 20 + offset);
      wrapper->show();
      activePlayers_.insert(name, wrapper);
      player->startStream();
    }
    QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  }
}
