#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QList>
#include <QMap>
#include <QString>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

#include "draggablewrapper.h"
#include "ros_worker.h"

namespace Ui {
class CameraWidget;
}

class CameraWidget : public QWidget {
  Q_OBJECT

public:
  explicit CameraWidget(RosWorker *rosWorker, QWidget *parent = nullptr);
  ~CameraWidget() override;

private slots:
  void onCamerasUpdated(CameraStatusList cameras);
  void onButtonClicked();

private:
  Ui::CameraWidget *ui;

  RosWorker *rosWorker_; // Shared from MainWindow

  QVBoxLayout *cameraButtonsLayout_;

  // Tracks the active video players on the canvas
  QMap<QString, DraggableWrapper *> activePlayers_;
};

#endif // CAMERAWIDGET_H
