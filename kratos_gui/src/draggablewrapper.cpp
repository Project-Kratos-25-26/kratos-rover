#include "draggablewrapper.h"

#include <QMouseEvent>
#include <QPainter>

DraggableWrapper::DraggableWrapper(QWidget *childWidget, QWidget *parent)
    : QWidget(parent), childWidget_(childWidget) {

  // Set up layout
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(4, 20, 4, 4); // Extra top margin for drag handle

  if (childWidget_) {
    layout_->addWidget(childWidget_);
  }

  setMouseTracking(true);
  setAttribute(Qt::WA_StyledBackground, true);
  setStyleSheet("DraggableWrapper { background-color: #2D2D2D; border: 1px "
                "solid #555555; border-radius: 4px; }");

  setMinimumSize(320, 240);
}

DraggableWrapper::~DraggableWrapper() {}

void DraggableWrapper::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QRect resizeRect(width() - resizeMargin_, height() - resizeMargin_,
                     resizeMargin_, resizeMargin_);
    QRect dragRect(0, 0, width(), 20); // Top bar for dragging

    if (resizeRect.contains(event->pos())) {
      isResizing_ = true;
      dragPosition_ = event->pos();
    } else if (dragRect.contains(event->pos())) {
      isDragging_ = true;
      dragPosition_ = event->pos();
    }
    event->accept();
  }
}

void DraggableWrapper::mouseMoveEvent(QMouseEvent *event) {
  updateCursorShape(event->pos());

  if (isDragging_) {
    move(mapToParent(event->pos() - dragPosition_));
    event->accept();
  } else if (isResizing_) {
    // Calculate the new size based on the movement of the mouse
    int deltaX = event->pos().x() - dragPosition_.x();
    int deltaY = event->pos().y() - dragPosition_.y();

    int newWidth = qMax(minimumWidth(), width() + deltaX);
    int newHeight = qMax(minimumHeight(), height() + deltaY);

    resize(newWidth, newHeight);

    // Update drag position to current pos after resize
    dragPosition_ = QPoint(event->pos().x() - deltaX + (newWidth - width()),
                           event->pos().y() - deltaY + (newHeight - height()));
    event->accept();
  }
}

void DraggableWrapper::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    isDragging_ = false;
    isResizing_ = false;
    event->accept();
  }
}

void DraggableWrapper::updateCursorShape(const QPoint &pos) {
  QRect resizeRect(width() - resizeMargin_, height() - resizeMargin_,
                   resizeMargin_, resizeMargin_);
  QRect dragRect(0, 0, width(), 20);

  if (isResizing_ || resizeRect.contains(pos)) {
    setCursor(Qt::SizeFDiagCursor);
  } else if (isDragging_ || dragRect.contains(pos)) {
    setCursor(Qt::SizeAllCursor);
  } else {
    setCursor(Qt::ArrowCursor);
  }
}

void DraggableWrapper::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);

  QPainter painter(this);

  // Draw drag handle indicator
  painter.setPen(QColor(120, 120, 120));
  painter.drawLine(width() / 2 - 15, 10, width() / 2 + 15, 10);
  painter.drawLine(width() / 2 - 15, 14, width() / 2 + 15, 14);

  // Draw resize handle
  painter.setPen(QColor(150, 150, 150));
  int w = width();
  int h = height();
  painter.drawLine(w - 12, h - 4, w - 4, h - 12);
  painter.drawLine(w - 8, h - 4, w - 4, h - 8);
}
