#ifndef DRAGGABLEWRAPPER_H
#define DRAGGABLEWRAPPER_H

#include <QMouseEvent>
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

  bool isDragging_ = false;
  bool isResizing_ = false;
  QPoint dragPosition_;

  const int resizeMargin_ = 15;

  void updateCursorShape(const QPoint &pos);
};

#endif // DRAGGABLEWRAPPER_H
