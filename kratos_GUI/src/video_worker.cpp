#include "video_worker.h"
#include <QDebug>
#include <gst/video/video.h>

VideoWorker::VideoWorker(QObject *parent) : QObject(parent) {
  gst_init(nullptr, nullptr);
}

VideoWorker::~VideoWorker() { stopPipeline(); }

void VideoWorker::startPipeline(const QString &device) {
  stopPipeline();

  QString pipeStr;
  bool ok;
  int port = device.toInt(&ok);
  if (ok && port > 0) {
    // treat as TCP port coming from kratos_cameras; the stream is served as a
    // Matroska/AV1 pipeline so we demux and decode before converting to RGB
    pipeStr = QString(
        "tcpclientsrc host=127.0.0.1 port=%1 ! matroskademux ! decodebin ! "
        "videoconvert ! videoscale ! video/x-raw,width=640,height=480,format=RGB ! "
        "appsink name=sink emit-signals=true sync=false")
                  .arg(port);
  } else {
    // Use v4l2src for Linux webcams.
    // Pipeline: v4l2src -> videoconvert -> videoscale -> capsfilter -> appsink
    pipeStr = QString("v4l2src device=%1 ! videoconvert ! videoscale ! "
                      "video/x-raw,width=640,height=480,format=RGB ! "
                      "appsink name=sink emit-signals=true sync=false")
                  .arg(device);
  }

  GError *error = nullptr;
  pipeline_ = gst_parse_launch(pipeStr.toUtf8().constData(), &error);

  if (error) {
    emit errorOccurred(
        QString("Failed to parse pipeline: %1").arg(error->message));
    g_error_free(error);
    return;
  }

  GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
  g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
  gst_object_unref(sink);

  gst_element_set_state(pipeline_, GST_STATE_PLAYING);
}

void VideoWorker::stopPipeline() {
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
}

GstFlowReturn VideoWorker::on_new_sample(GstAppSink *sink, gpointer user_data) {
  VideoWorker *worker = static_cast<VideoWorker *>(user_data);
  GstSample *sample = gst_app_sink_pull_sample(sink);

  if (sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo info;

    if (gst_video_info_from_caps(&info, caps)) {
      GstMapInfo map;
      if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        // Create QImage from buffer data
        // Note: We copy the data here to be safe since the buffer will be
        // unmapped
        QImage img(map.data, info.width, info.height, QImage::Format_RGB888);
        emit worker->newFrame(img.copy());
        gst_buffer_unmap(buffer, &map);
      }
    }
    gst_sample_unref(sample);
  }

  return GST_FLOW_OK;
}
