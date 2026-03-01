#include "draggablewrapper.h"
#include <QPainter>
#include <QPen>

DraggableWrapper::DraggableWrapper(QWidget *childWidget, QWidget *parent)
    : QWidget(parent), childWidget_(childWidget), isDragging_(false),
      isResizing_(false) {

  // We want to draw our own resize handle and handle mouse events
  setAttribute(Qt::WA_StyledBackground, true);

  // Layout to hold the child widget
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(borderMargin_, borderMargin_, borderMargin_,
                              borderMargin_);
  layout_->addWidget(childWidget_);

  setMinimumSize(320, 240); // Minimum sensible video size

  // Optional: Add a styled border that highlights on hover (could be done via
  // CSS, but we do custom painting for the corner handle)
  setStyleSheet(R"(
        DraggableWrapper {
            background-color: transparent;
            border: 2px solid transparent;
        }
        DraggableWrapper:hover {
            border: 2px solid #555555;
        }
    )");

  // Enable mouse tracking so we can change cursors but normally hover is enough
  // setMouseTracking(true);
}

DraggableWrapper::~DraggableWrapper() {}

void DraggableWrapper::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    if (event->pos().x() > width() - borderMargin_ &&
        event->pos().y() > height() - borderMargin_) {
      // Bottom-right corner clicked -> start resizing
      isResizing_ = true;
      resizeStartGeometry_ = geometry();
      dragStartPosition_ = event->globalPosition().toPoint();
    } else {
      // Clicked inside -> start dragging
      isDragging_ = true;
      dragStartPosition_ = event->globalPosition().toPoint() - pos();
      this->raise(); // Bring to front
    }
  }
}

void DraggableWrapper::mouseMoveEvent(QMouseEvent *event) {
  if (isDragging_) {
    // Move the window
    move(event->globalPosition().toPoint() - dragStartPosition_);
  } else if (isResizing_) {
    // Resize the window
    int dx = event->globalPosition().toPoint().x() - dragStartPosition_.x();
    int dy = event->globalPosition().toPoint().y() - dragStartPosition_.y();

    int newWidth = qMax(minimumWidth(), resizeStartGeometry_.width() + dx);
    int newHeight = qMax(minimumHeight(), resizeStartGeometry_.height() + dy);

    resize(newWidth, newHeight);
  }
}

void DraggableWrapper::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    isDragging_ = false;
    isResizing_ = false;
  }
}

void DraggableWrapper::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);

  // Draw a subtle resize handle in the bottom right corner
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QPen pen(QColor("#888888"));
  pen.setWidth(2);
  painter.setPen(pen);

  // Draw two little diagonal lines
  painter.drawLine(width() - 8, height() - 2, width() - 2, height() - 8);
  painter.drawLine(width() - 14, height() - 2, width() - 2, height() - 14);
}
