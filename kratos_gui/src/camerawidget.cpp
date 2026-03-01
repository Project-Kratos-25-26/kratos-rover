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
    // If we receive an empty list from the backend, it means no cameras are
    // available. We should probably shut them all down except ZED (which
    // bypasses this). Let's iterate through activePlayers_ and close non-ZED
    // ones.
    QList<QString> toRemove;
    for (auto it = activePlayers_.constBegin(); it != activePlayers_.constEnd();
         ++it) {
      if (it.key() != "ZED") {
        toRemove.append(it.key());
      }
    }
    for (const QString &name : toRemove) {
      DraggableWrapper *wrapper = activePlayers_.take(name);
      if (wrapper)
        wrapper->deleteLater();
    }
  } else {
    for (const auto &camInfo : cameras) {
      QString name = camInfo.name;
      bool isActive = camInfo.active;
      int port = camInfo.port;

      if (name == "ZED") {
        isActive = activePlayers_.contains("ZED");
      } else {
        // Force backend active state sync for generic cameras
        if (isActive && !activePlayers_.contains(name)) {
          // It's active on backend, but we don't have it displayed! We should
          // auto-start it. (Or we just trust our local UI state and wait for
          // user click). For now, let's keep local state as source of truth for
          // display
          isActive = activePlayers_.contains(name);
        } else if (!isActive && activePlayers_.contains(name)) {
          // Backend says false, but we have it. Close it.
          DraggableWrapper *wrapper = activePlayers_.take(name);
          if (wrapper)
            wrapper->deleteLater();
        }
      }

      QPushButton *btn = new QPushButton(name, ui->scrollAreaWidgetContents);
      btn->setProperty("cameraName", name);
      btn->setProperty("isActive", isActive);
      btn->setProperty("port", port);

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
