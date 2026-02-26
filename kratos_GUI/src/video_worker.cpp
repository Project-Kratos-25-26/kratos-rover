#include "video_worker.h"
#include <QDebug>
#include <gst/video/video.h>

VideoWorker::VideoWorker(QObject *parent) : QObject(parent) {
  qDebug() << "[VIDEO_WORKER] Initializing GStreamer...";
  gst_init(nullptr, nullptr);
  qDebug() << "[VIDEO_WORKER] GStreamer initialized successfully";
}

VideoWorker::~VideoWorker() {
  qDebug() << "[VIDEO_WORKER] Destructor: stopping any active pipeline";
  stopPipeline();
  qDebug() << "[VIDEO_WORKER] Destroyed";
}

void VideoWorker::startPipeline(const QString &device) {
  qDebug() << "[VIDEO_PIPELINE] startPipeline called with:" << device;
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
    qDebug() << "[VIDEO_PIPELINE] TCP stream mode - connecting to port" << port;
  } else {
    // Use v4l2src for Linux webcams.
    // Pipeline: v4l2src -> videoconvert -> videoscale -> capsfilter -> appsink
    pipeStr = QString("v4l2src device=%1 ! videoconvert ! videoscale ! "
                      "video/x-raw,width=640,height=480,format=RGB ! "
                      "appsink name=sink emit-signals=true sync=false")
                  .arg(device);
    qDebug() << "[VIDEO_PIPELINE] Local v4l2 device mode:" << device;
  }

  GError *error = nullptr;
  qDebug() << "[VIDEO_PIPELINE] Parsing pipeline string...";
  pipeline_ = gst_parse_launch(pipeStr.toUtf8().constData(), &error);

  if (error) {
    QString errMsg = QString("[VIDEO_PIPELINE_ERROR] Failed to parse pipeline: %1").arg(error->message);
    qWarning() << errMsg;
    emit errorOccurred(errMsg);
    g_error_free(error);
    return;
  }

  qDebug() << "[VIDEO_PIPELINE] Pipeline created successfully";

  GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
  g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
  gst_object_unref(sink);

  qDebug() << "[VIDEO_PIPELINE] Setting state to PLAYING...";
  GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    qWarning() << "[VIDEO_PIPELINE_ERROR] Failed to set pipeline to PLAYING state";
    emit errorOccurred("Failed to start video stream");
    stopPipeline();
    return;
  }
  qDebug() << "[VIDEO_PIPELINE] Pipeline is now PLAYING";
}

void VideoWorker::stopPipeline() {
  if (pipeline_) {
    qDebug() << "[VIDEO_PIPELINE] Stopping pipeline...";
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
    qDebug() << "[VIDEO_PIPELINE] Pipeline stopped and destroyed";
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
