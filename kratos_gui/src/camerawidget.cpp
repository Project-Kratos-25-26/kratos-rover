#include "camerawidget.h"
#include "ui_camerawidget.h"

#include <QDebug>
#include <QLabel>
#include <QScrollArea>

// ─── Layout options ────────────────────────────────────────────────
// Each entry: "label" → {rows, cols}
static const struct {
  const char *label;
  int rows, cols;
} kLayouts[] = {
    {"1 × 1", 1, 1}, {"2 × 1", 2, 1}, {"2 × 2", 2, 2},
    {"3 × 2", 3, 2}, {"3 × 3", 3, 3}, {"4 × 4", 4, 4},
};
static constexpr int kLayoutCount = sizeof(kLayouts) / sizeof(kLayouts[0]);

// ─── Size presets (fixed cell sizes in px) ─────────────────────────
static const struct {
  const char *label;
  int width, height; // 0,0 = auto-fit (stretch)
} kSizes[] = {
    {"Auto-fit", 0, 0},
    {"Small (320×240)", 320, 240},
    {"Medium (640×480)", 640, 480},
    {"Large (960×720)", 960, 720},
    {"XL (1280×720)", 1280, 720},
};
static constexpr int kSizeCount = sizeof(kSizes) / sizeof(kSizes[0]);

// ─── Shared stylesheet for combo boxes ─────────────────────────────
static const char *kComboStyle = R"(
    QComboBox {
        color: #CCCCCC;
        background-color: #252525;
        border: 1px solid #444444;
        border-radius: 3px;
        padding: 3px 8px;
        font-size: 11px;
        min-width: 120px;
    }
    QComboBox:hover { border: 1px solid #FFFF55; }
    QComboBox::drop-down {
        border: none;
        width: 18px;
    }
    QComboBox::down-arrow {
        image: none;
        border-left: 4px solid transparent;
        border-right: 4px solid transparent;
        border-top: 5px solid #AAAAAA;
        margin-right: 5px;
    }
    QComboBox QAbstractItemView {
        background-color: #1C1C1C;
        color: #CCCCCC;
        selection-background-color: #333333;
        selection-color: #FFFF55;
        border: 1px solid #444444;
        font-size: 11px;
    }
)";

// Small combo for per-cell stream picker
static const char *kCellComboStyle = R"(
    QComboBox {
        color: #BBBBBB;
        background-color: rgba(30, 30, 30, 200);
        border: 1px solid #555555;
        border-radius: 2px;
        padding: 2px 6px;
        font-size: 10px;
        min-width: 90px;
        max-width: 200px;
    }
    QComboBox:hover { border: 1px solid #FFFF55; }
    QComboBox::drop-down { border: none; width: 14px; }
    QComboBox::down-arrow {
        image: none;
        border-left: 3px solid transparent;
        border-right: 3px solid transparent;
        border-top: 4px solid #999999;
        margin-right: 4px;
    }
    QComboBox QAbstractItemView {
        background-color: #1C1C1C;
        color: #CCCCCC;
        selection-background-color: #333333;
        selection-color: #FFFF55;
        border: 1px solid #444444;
        font-size: 10px;
    }
)";

// ────────────────────────────────────────────────────────────────────

CameraWidget::CameraWidget(RosWorker *rosWorker, QWidget *parent)
    : QWidget(parent), ui(new Ui::CameraWidget), rosWorker_(rosWorker) {
  ui->setupUi(this);

  // ── Build toolbar combos ─────────────────────────────────────────
  sidebarSplitToggle_ = new QPushButton("☰", this);
  sidebarSplitToggle_->setStyleSheet(
      "QPushButton { color: #AAAAAA; background: transparent; border: none; font-size: 16px; }"
      "QPushButton:hover { color: #FFFF55; }"
  );
  sidebarSplitToggle_->setFixedSize(30, 30);
  connect(sidebarSplitToggle_, &QPushButton::clicked, this, &CameraWidget::onSidebarToggle);

  QLabel *layoutLabel = new QLabel("LAYOUT", this);
  layoutLabel->setStyleSheet(
      "QLabel { color: #888888; font-size: 10px; font-weight: bold; }");

  layoutCombo_ = new QComboBox(this);
  layoutCombo_->setStyleSheet(kComboStyle);
  for (int i = 0; i < kLayoutCount; ++i)
    layoutCombo_->addItem(kLayouts[i].label);
  layoutCombo_->setCurrentIndex(2); // default 2×2

  QLabel *sizeLabel = new QLabel("SIZE", this);
  sizeLabel->setStyleSheet(
      "QLabel { color: #888888; font-size: 10px; font-weight: bold; }");

  sizeCombo_ = new QComboBox(this);
  sizeCombo_->setStyleSheet(kComboStyle);
  for (int i = 0; i < kSizeCount; ++i)
    sizeCombo_->addItem(kSizes[i].label);
  sizeCombo_->setCurrentIndex(0); // default auto-fit

  // Insert into toolbar (before the spacer, which is already at index 0)
  QHBoxLayout *toolbar = ui->toolbarLayout;
  toolbar->insertWidget(0, sidebarSplitToggle_);
  toolbar->insertWidget(1, layoutLabel);
  toolbar->insertWidget(2, layoutCombo_);
  toolbar->insertSpacing(3, 16);
  toolbar->insertWidget(4, sizeLabel);
  toolbar->insertWidget(5, sizeCombo_);

  connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &CameraWidget::onLayoutChanged);
  connect(sizeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &CameraWidget::onSizeChanged);

  // ── Setup Sidebar ────────────────────────────────────────────────
  camerasListLayout_ =
      qobject_cast<QVBoxLayout *>(ui->sidebarScrollContents->layout());
  
  // Set default splitter sizes (e.g. 250px sidebar, remainder to grid)
  ui->mainSplitter->setSizes(QList<int>() << 250 << 800);

  // ── Build grid area ──────────────────────────────────────────────
  gridContainer_ = new QWidget(this);
  gridContainer_->setStyleSheet(
      "QWidget { background-color: #0A0A0A; }");
  gridLayout_ = new QGridLayout(gridContainer_);
  gridLayout_->setSpacing(4);
  gridLayout_->setContentsMargins(4, 4, 4, 4);

  ui->gridScrollArea->setWidget(gridContainer_);

  // ── ROS connection ───────────────────────────────────────────────
  connect(rosWorker_, &RosWorker::camerasUpdated, this,
          &CameraWidget::onCamerasUpdated);

  // Build initial grid (2×2)
  rebuildGrid(2, 2);

  setupHotkeys();
}

CameraWidget::~CameraWidget() {
  // Stop all active streams
  for (auto &cell : cells_) {
    if (cell.player) {
      cell.player->stopStream();
    }
  }
  delete ui;
}

// ─── Parse layout index to rows/cols ───────────────────────────────

void CameraWidget::parseLayout(int index, int &rows, int &cols) {
  if (index < 0 || index >= kLayoutCount) {
    rows = 2;
    cols = 2;
    return;
  }
  rows = kLayouts[index].rows;
  cols = kLayouts[index].cols;
}

// ─── Rebuild the grid ──────────────────────────────────────────────

void CameraWidget::rebuildGrid(int rows, int cols) {
  // Stop streams and clean up existing cells
  for (auto &cell : cells_) {
    if (cell.player && !cell.assignedCamera.isEmpty()) {
      cell.player->stopStream();
    }
    // Deleting the frame deletes all children (player, picker) too
    if (cell.frame) {
      gridLayout_->removeWidget(cell.frame);
      delete cell.frame;
    }
  }
  cells_.clear();

  // Reset all existing stretch factors to 0
  for (int r = 0; r < gridLayout_->rowCount(); ++r)
    gridLayout_->setRowStretch(r, 0);
  for (int c = 0; c < gridLayout_->columnCount(); ++c)
    gridLayout_->setColumnStretch(c, 0);

  // Also reset row/column minimum sizes by removing any leftover spacers
  // Recreate the layout entirely for a clean slate
  delete gridLayout_;
  gridLayout_ = new QGridLayout(gridContainer_);
  gridLayout_->setSpacing(4);
  gridLayout_->setContentsMargins(4, 4, 4, 4);

  int totalCells = rows * cols;
  cells_.resize(totalCells);

  // Size settings
  int sizeIdx = sizeCombo_->currentIndex();
  bool autoFit = (sizeIdx == 0);
  int cellW = autoFit ? 0 : kSizes[sizeIdx].width;
  int cellH = autoFit ? 0 : kSizes[sizeIdx].height;

  for (int i = 0; i < totalCells; ++i) {
    int r = i / cols;
    int c = i % cols;

    // Create a container for the cell (player + overlay combo)
    QWidget *cellFrame = new QWidget(gridContainer_);
    cellFrame->setStyleSheet(
        "QWidget { background-color: #121212; border: 1px solid #2A2A2A; }");

    QVBoxLayout *cellLayout = new QVBoxLayout(cellFrame);
    cellLayout->setContentsMargins(0, 0, 0, 0);
    cellLayout->setSpacing(0);

    // Stream picker combo at the top of each cell
    QComboBox *picker = new QComboBox(cellFrame);
    picker->setStyleSheet(kCellComboStyle);
    picker->addItem("— None —");
    // Populate with known cameras
    for (const auto &cam : knownCameras_) {
      picker->addItem(cam.name);
    }
    picker->setProperty("cellIndex", i);
    connect(picker, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, i](int) { onCellStreamChanged(i); });
    cellLayout->addWidget(picker);

    // Video player
    VideoPlayerWidget *player =
        new VideoPlayerWidget("", 0, rosWorker_, cellFrame);
    cellLayout->addWidget(player, 1); // stretch factor 1

    if (!autoFit) {
      cellFrame->setFixedSize(cellW, cellH);
    } else {
      cellFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      cellFrame->setMinimumSize(200, 150);
    }

    gridLayout_->addWidget(cellFrame, r, c);

    // Set equal stretch for auto-fit
    if (autoFit) {
      gridLayout_->setRowStretch(r, 1);
      gridLayout_->setColumnStretch(c, 1);
    }

    cells_[i].frame = cellFrame;
    cells_[i].player = player;
    cells_[i].streamPicker = picker;
    cells_[i].assignedCamera = "";
  }

  // Auto-allocate active cameras to new cells
  autoAssignCameras();
}

// ─── Layout changed ────────────────────────────────────────────────

void CameraWidget::onLayoutChanged(int index) {
  int rows, cols;
  parseLayout(index, rows, cols);
  rebuildGrid(rows, cols);
}

// ─── Size changed ──────────────────────────────────────────────────

void CameraWidget::onSizeChanged(int /*index*/) {
  // Rebuild with current layout to apply new size
  int rows, cols;
  parseLayout(layoutCombo_->currentIndex(), rows, cols);
  rebuildGrid(rows, cols);
}

// ─── Camera list from ROS ──────────────────────────────────────────

void CameraWidget::onCamerasUpdated(CameraStatusList cameras) {
  // Check if active cameras changed to trigger auto-assignment
  bool activeChanged = false;
  int oldActiveCount = 0;
  int newActiveCount = 0;
  for (const auto& c : knownCameras_) if (c.active) oldActiveCount++;
  for (const auto& c : cameras) if (c.active) newActiveCount++;
  
  if (oldActiveCount != newActiveCount) {
    activeChanged = true;
  } else {
    // Counts match, but did the identities change?
    for (int i=0; i<cameras.size(); ++i) {
      int oldIdx = knownCameras_.indexOf(cameras[i]);
      if (oldIdx >= 0 && knownCameras_[oldIdx].active != cameras[i].active) {
        activeChanged = true;
        break;
      }
    }
  }

  knownCameras_ = cameras;
  updateCellDropdowns();
  
  // Clear existing sidebar buttons
  QLayoutItem *child;
  for (int i = camerasListLayout_->count() - 1; i >= 0; --i) {
    child = camerasListLayout_->itemAt(i);
    if (child->widget()) {
      delete child->widget();
    }
  }

  // Populate sidebar buttons
  for (const auto &camInfo : knownCameras_) {
    QPushButton *btn = new QPushButton(camInfo.name, ui->sidebarScrollContents);
    btn->setProperty("cameraName", camInfo.name);
    btn->setProperty("isActive", camInfo.active);
    btn->setProperty("port", camInfo.port);

    QString btnStyle = R"(
        QPushButton { 
            color: #AAAAAA; padding: 8px 12px; border: 1px solid #333333; 
            border-radius: 0px; font-size: 11px; background-color: #1C1C1C; 
            text-align: left;
        }
        QPushButton:hover { background-color: #252525; color: #FFFFFF; }
    )";

    if (camInfo.active) {
      btn->setStyleSheet(R"(
          QPushButton { 
              color: #FFFF55; padding: 8px 12px; border: 1px solid #FFFF55; 
              border-radius: 0px; font-size: 11px; background-color: #252525; 
              text-align: left;
          }
          QPushButton:hover { background-color: #333333; }
      )");
      btn->setText("■ " + camInfo.name + " (ACTIVE)");
    } else {
      btn->setStyleSheet(btnStyle);
      btn->setText("▶ " + camInfo.name + " (OFF)");
    }

    connect(btn, &QPushButton::clicked, this, &CameraWidget::onSidebarCameraClicked);
    // Insert before spacer
    camerasListLayout_->insertWidget(camerasListLayout_->count() - 1, btn);
  }

  // Auto-allocate whenever active cameras change
  if (activeChanged) {
    autoAssignCameras();
  }
}

// ─── Sidebar actions ───────────────────────────────────────────────

void CameraWidget::onSidebarToggle() {
  QList<int> sizes = ui->mainSplitter->sizes();
  if (sizes.value(0, 0) == 0) {
    // Hidden -> Show (restore to 250px)
    ui->mainSplitter->setSizes(QList<int>() << 250 << ui->mainSplitter->width() - 250);
  } else {
    // Shown -> Hide
    ui->mainSplitter->setSizes(QList<int>() << 0 << ui->mainSplitter->width());
  }
}

void CameraWidget::onSidebarCameraClicked() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;

  QString name = btn->property("cameraName").toString();
  bool isActive = btn->property("isActive").toBool();

  if (isActive) {
    // Unassign from any cells displaying this stream
    for (int i = 0; i < cells_.size(); ++i) {
      if (cells_[i].assignedCamera == name) {
        cells_[i].streamPicker->setCurrentIndex(0); // "None"
      }
    }
    QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  } else {
    QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                              Qt::QueuedConnection, Q_ARG(QString, name));
  }
}

// ─── Hotkeys ───────────────────────────────────────────────────────

void CameraWidget::setupHotkeys() {
  // Ctrl + 1..9 to toggle camera 1..9
  for (int i = 0; i < 9; ++i) {
    QShortcut *shortcut = new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this);
    connect(shortcut, &QShortcut::activated, this, [this, i]() {
      toggleCameraByIndex(i);
    });
  }

  // Ctrl + A to toggle all cameras
  QShortcut *toggleAllShortcut = new QShortcut(QKeySequence("Ctrl+A"), this);
  connect(toggleAllShortcut, &QShortcut::activated, this, &CameraWidget::toggleAllCameras);

  // Ctrl + R to focus layout dropdown
  QShortcut *focusLayoutShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
  connect(focusLayoutShortcut, &QShortcut::activated, this, [this]() {
    if (layoutCombo_) {
      layoutCombo_->setFocus();
      layoutCombo_->showPopup();
    }
  });
}

void CameraWidget::toggleCameraByIndex(int index) {
  if (index < 0 || index >= knownCameras_.size())
    return;

  // We want to simulate clicking the corresponding sidebar button.
  // The sidebar buttons are in camerasListLayout_ and correspond 1:1 with knownCameras_
  // but let's just trigger the service call directly based on knownCameras_[index] state
  const CameraInfo &cam = knownCameras_[index];
  
  if (cam.active) {
    // Unassign from any cells displaying this stream
    for (int i = 0; i < cells_.size(); ++i) {
      if (cells_[i].assignedCamera == cam.name) {
        cells_[i].streamPicker->setCurrentIndex(0); // "None"
      }
    }
    QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                              Qt::QueuedConnection, Q_ARG(QString, cam.name));
  } else {
    QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                              Qt::QueuedConnection, Q_ARG(QString, cam.name));
  }
}

void CameraWidget::toggleAllCameras() {
  if (knownCameras_.isEmpty()) return;

  // Check if *all* are currently active
  bool allActive = true;
  for (const auto &cam : knownCameras_) {
    if (!cam.active) {
      allActive = false;
      break;
    }
  }

  if (allActive) {
    // Turn all OFF
    for (const auto &cam : knownCameras_) {
      for (int i = 0; i < cells_.size(); ++i) {
        if (cells_[i].assignedCamera == cam.name) {
          cells_[i].streamPicker->setCurrentIndex(0);
        }
      }
      QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                                Qt::QueuedConnection, Q_ARG(QString, cam.name));
    }
  } else {
    // Turn all ON (that are currently off)
    for (const auto &cam : knownCameras_) {
      if (!cam.active) {
        QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                                  Qt::QueuedConnection, Q_ARG(QString, cam.name));
      }
    }
  }
}

void CameraWidget::pauseAllStreams() {
  for (int i = 0; i < cells_.size(); ++i) {
    if (!cells_[i].assignedCamera.isEmpty() && cells_[i].player) {
      cells_[i].player->stopStream();
      QMetaObject::invokeMethod(rosWorker_, "callStopStream",
                                Qt::QueuedConnection,
                                Q_ARG(QString, cells_[i].assignedCamera));
    }
  }
}

void CameraWidget::resumeAllStreams() {
  for (int i = 0; i < cells_.size(); ++i) {
    if (!cells_[i].assignedCamera.isEmpty() && cells_[i].player) {
      // Find the port for the assigned camera to restart
      int pt = 5000; // default
      for (const auto &cam : knownCameras_) {
        if (cam.name == cells_[i].assignedCamera) {
          pt = cam.port;
          break;
        }
      }
      cells_[i].player->switchStream(cells_[i].assignedCamera, pt);
      QMetaObject::invokeMethod(rosWorker_, "callStartStream",
                                Qt::QueuedConnection,
                                Q_ARG(QString, cells_[i].assignedCamera));
    }
  }
}

// ─── Update all cell dropdowns with current camera list ────────────

void CameraWidget::updateCellDropdowns() {
  for (int i = 0; i < cells_.size(); ++i) {
    QComboBox *picker = cells_[i].streamPicker;
    if (!picker)
      continue;

    // Save current selection
    QString currentSelection = cells_[i].assignedCamera;

    // Block signals while rebuilding items
    picker->blockSignals(true);
    picker->clear();
    picker->addItem("— None —");

    int restoreIndex = 0;
    for (int j = 0; j < knownCameras_.size(); ++j) {
      picker->addItem(knownCameras_[j].name);
      if (knownCameras_[j].name == currentSelection) {
        restoreIndex = j + 1; // +1 because "None" is at index 0
      }
    }

    picker->setCurrentIndex(restoreIndex);
    picker->blockSignals(false);
  }
}

// ─── User changed which stream a cell shows ────────────────────────

void CameraWidget::onCellStreamChanged(int cellIndex) {
  if (cellIndex < 0 || cellIndex >= cells_.size())
    return;

  GridCell &cell = cells_[cellIndex];
  QComboBox *picker = cell.streamPicker;
  if (!picker || !cell.player)
    return;

  int pickerIndex = picker->currentIndex();

  // Stop old stream if any
  if (!cell.assignedCamera.isEmpty()) {
    cell.player->stopStream();
    cell.assignedCamera = "";
  }

  // "None" selected
  if (pickerIndex <= 0) {
    return;
  }

  // Start new stream locally
  int camIndex = pickerIndex - 1; // offset for "None"
  if (camIndex < 0 || camIndex >= knownCameras_.size())
    return;

  const CameraInfo &cam = knownCameras_[camIndex];
  cell.assignedCamera = cam.name;

  cell.player->switchStream(cam.name, cam.port);
}

void CameraWidget::autoAssignCameras() {
  int nextCellIdx = 0;
  for (const auto &cam : knownCameras_) {
    if (cam.active && nextCellIdx < cells_.size()) {
      QComboBox *picker = cells_[nextCellIdx].streamPicker;
      if (picker) {
        int camIdx = knownCameras_.indexOf(cam) + 1;
        picker->setCurrentIndex(camIdx); // Triggers onCellStreamChanged to start stream in UI
      }
      nextCellIdx++;
    }
  }
}
