#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H

#include <QImage>
#include <QObject>
#include <QString>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>

class VideoWorker : public QObject {
  Q_OBJECT

public:
  explicit VideoWorker(QObject *parent = nullptr);
  ~VideoWorker();

public slots:
  void startPipeline(const QString &device);
  void stopPipeline();

signals:
  void newFrame(const QImage &frame);
  void errorOccurred(const QString &message);

private:
  static GstFlowReturn on_new_sample(GstAppSink *sink, gpointer user_data);

  GstElement *pipeline_ = nullptr;
};

#endif // VIDEO_WORKER_H
