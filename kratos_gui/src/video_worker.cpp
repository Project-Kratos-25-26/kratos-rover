#include "video_worker.h"

#include <QDebug>
#include <gst/video/video.h>

VideoWorker::VideoWorker(QObject *parent) : QObject(parent) {
  // Ensure GStreamer is initialized (safe to call multiple times)
  if (!gst_is_initialized()) {
    gst_init(nullptr, nullptr);
  }
}

VideoWorker::~VideoWorker() { stopPipeline(); }

// ─── Start Pipeline ────────────────────────────────────────────────

void VideoWorker::startPipeline(QString host, int port) {
  // Tear down any existing pipeline first
  stopPipeline();

  // AV1 decoding over TCP (Matroska container)
  // Jetson sender uses: nvv4l2av1enc ! av1parse ! matroskamux ! tcpserversink
  // GUI receives via TCP client, demuxes Matroska, decodes with av1dec (libaom, CPU)
  
  QString pipelineStr =
      QString("tcpclientsrc host=%1 port=%2 "
              "! matroskademux "
              "! av1parse "
              "! av1dec "
              "! videoconvert "
              "! videoscale "
              "! video/x-raw,width=1280,height=720,format=RGB "
              "! appsink name=sink emit-signals=true sync=false")
          .arg(host)
          .arg(port);

  GError *error = nullptr;
  pipeline_ = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);

  if (error) {
    emit pipelineError(QString("Pipeline parse error: %1").arg(error->message));
    g_error_free(error);
    return;
  }

  // Get the appsink element and wire up the new-sample callback
  GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
  if (!sink) {
    emit pipelineError("Could not find appsink element 'sink'");
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
    return;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.new_sample = &VideoWorker::onNewSample;
  gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, this, nullptr);
  gst_object_unref(sink);

  // Start the pipeline
  GstStateChangeReturn ret =
      gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    emit pipelineError("Failed to set pipeline to PLAYING");
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
}

// ─── Stop Pipeline ─────────────────────────────────────────────────

void VideoWorker::stopPipeline() {
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
}

// ─── GStreamer Callback (static) ───────────────────────────────────

GstFlowReturn VideoWorker::onNewSample(GstAppSink *sink, gpointer userData) {
  auto *self = static_cast<VideoWorker *>(userData);

  GstSample *sample = gst_app_sink_pull_sample(sink);
  if (!sample) {
    return GST_FLOW_ERROR;
  }

  GstBuffer *buffer = gst_sample_get_buffer(sample);
  GstCaps *caps = gst_sample_get_caps(sample);

  if (!buffer || !caps) {
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  // Extract width/height from caps
  GstVideoInfo info;
  gst_video_info_init(&info);
  if (!gst_video_info_from_caps(&info, caps)) {
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  int width = GST_VIDEO_INFO_WIDTH(&info);
  int height = GST_VIDEO_INFO_HEIGHT(&info);

  // Map the buffer to read pixel data
  GstMapInfo mapInfo;
  if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  // Create a deep-copied QImage (RGB888 matches format=RGB in pipeline)
  QImage frame(mapInfo.data, width, height,
               GST_VIDEO_INFO_PLANE_STRIDE(&info, 0), QImage::Format_RGB888);
  QImage copied = frame.copy(); // deep copy — data valid after unmap

  gst_buffer_unmap(buffer, &mapInfo);
  gst_sample_unref(sample);

  emit self->newFrame(copied);

  return GST_FLOW_OK;
}
