#ifndef DRAGGABLEWRAPPER_H
#define DRAGGABLEWRAPPER_H

#include <QMouseEvent>
#include <QPoint>
#include <QVBoxLayout>
#include <QWidget>

class DraggableWrapper : public QWidget {
  Q_OBJECT

public:
  explicit DraggableWrapper(QWidget *childWidget, QWidget *parent = nullptr);
  ~DraggableWrapper() override;

  QWidget *getChildWidget() const { return childWidget_; }

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  QWidget *childWidget_;
  QVBoxLayout *layout_;

  bool isDragging_;
  bool isResizing_;
  QPoint dragStartPosition_;
  QRect resizeStartGeometry_;

  int borderMargin_ = 10; // Margin around the edge to trigger resize
};

#endif // DRAGGABLEWRAPPER_H
