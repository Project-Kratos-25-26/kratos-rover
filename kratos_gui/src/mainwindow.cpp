#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QStyle>
#include <QTabBar>

#include "camerawidget.h"
#include "joynodewidget.h"

static bool _registered = []() {
  qRegisterMetaType<CameraStatusList>("CameraStatusList");
  qRegisterMetaType<QImage>("QImage");
  qRegisterMetaType<QList<float>>("QList<float>");
  qRegisterMetaType<QList<int>>("QList<int>");
  return true;
}();

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Styling for Nvidia-Isaac Sim dark aesthetic
  setStyleSheet(R"(
    QMainWindow { 
        background-color: #121212; 
        font-family: 'Roboto', 'Segoe UI', Arial, sans-serif;
    }
    QPushButton#bigAddBtn {
        color: #FFFF55;
        background-color: #1C1C1C;
        border: 1px solid #333333;
        border-radius: 0px;
        font-size: 14px;
        padding: 10px 20px;
    }
    QPushButton#bigAddBtn:hover {
        border: 1px solid #FFFF55;
        background-color: #252525;
    }
    QPushButton#bigAddBtn::menu-indicator {
        image: none;
    }
    QTabWidget::pane {
        border-top: 1px solid #333333;
        background-color: #121212;
    }
    QTabBar {
        background-color: #000000;
    }
    QTabBar::tab {
        background-color: #1C1C1C;
        color: #AAAAAA;
        border: 1px solid #333333;
        border-radius: 0px;
        padding: 5px 12px;
        margin-right: 1px;
        font-size: 11px;
    }
    QTabBar::tab:selected {
        color: #FFFFFF;
        background-color: #252525;
        border-top: 1px solid #FFFF55;
        border-bottom: 1px solid #252525;
    }
    QTabBar::tab:hover:!selected {
        color: #CCCCCC;
        background-color: #2A2A2A;
    }
    QStatusBar { 
        background: #1C1C1C; 
        color: #AAAAAA; 
        border-top: 1px solid #333333;
        font-size: 11px;
    }
    QMenu {
        background-color: #1C1C1C;
        color: #CCCCCC;
        border: 1px solid #333333;
        border-radius: 0px;
        font-size: 11px;
    }
    QMenu::item {
        padding: 6px 20px;
    }
    QMenu::item:selected {
        background-color: #252525;
        color: #FFFF55;
        border-left: 2px solid #FFFF55;
    }
  )");

  setupAddMenu(ui->bigAddBtn);

  // Setup Dummy Add Tab
  int addTabIndex = ui->tabWidget->addTab(new QWidget(), " + ");
  QTabBar *tabBar = ui->tabWidget->findChild<QTabBar *>();
  if (tabBar) {
    tabBar->setTabButton(addTabIndex, QTabBar::RightSide, nullptr);
  }

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
    if (index == ui->tabWidget->count() - 1 && index >= 0) {
      int prev = (ui->tabWidget->count() > 1) ? index - 1 : -1;
      if (prev >= 0) {
        ui->tabWidget->setCurrentIndex(prev);
      } else {
        ui->stackedWidget->setCurrentIndex(0);
      }
    }
  });

  connect(ui->tabWidget, &QTabWidget::tabBarClicked, this, [this](int index) {
    if (index == ui->tabWidget->count() - 1) {
      QMenu addMenu(this);
      addMenu.setStyleSheet(this->styleSheet());
      connect(addMenu.addAction("Add Cam View"), &QAction::triggered, this,
              &MainWindow::onAddCamClicked);
      connect(addMenu.addAction("Add Joynode Panel"), &QAction::triggered, this,
              &MainWindow::onAddJoyNodeClicked);
      addMenu.exec(QCursor::pos());
    }
  });

  connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this,
          &MainWindow::onTabCloseRequested);

  ui->stackedWidget->setCurrentIndex(0); // Show empty state initially

  // Set up ROS Worker
  rosWorker_ = new RosWorker();
  rosWorker_->moveToThread(&rosThread_);
  connect(&rosThread_, &QThread::started, rosWorker_, &RosWorker::init);
  connect(&rosThread_, &QThread::finished, rosWorker_, &QObject::deleteLater);

  rosThread_.start();
}

MainWindow::~MainWindow() {
  rosThread_.quit();
  rosThread_.wait();

  delete ui;
}

void MainWindow::setupAddMenu(QPushButton *btn) {
  QMenu *addMenu = new QMenu(this);
  connect(addMenu->addAction("Add Cam View"), &QAction::triggered, this,
          &MainWindow::onAddCamClicked);
  connect(addMenu->addAction("Add Joynode Panel"), &QAction::triggered, this,
          &MainWindow::onAddJoyNodeClicked);
  btn->setMenu(addMenu);
}

void MainWindow::onAddCamClicked() {
  ui->stackedWidget->setCurrentIndex(1); // Switch to tabs view
  int targetIndex = ui->tabWidget->count() - 1;
  if (targetIndex < 0)
    targetIndex = 0;

  int nextNum = 1;
  while (true) {
    bool numUsed = false;
    QString nameToCheck = QString("CAMS %1").arg(nextNum);
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
      if (ui->tabWidget->tabText(i) == nameToCheck) {
        numUsed = true;
        break;
      }
    }
    if (!numUsed)
      break;
    nextNum++;
  }

  CameraWidget *cw = new CameraWidget(rosWorker_, this);
  int index = ui->tabWidget->insertTab(targetIndex, cw,
                                       QString("CAMS %1").arg(nextNum));

  QTabBar *tabBar = ui->tabWidget->findChild<QTabBar *>();
  if (tabBar) {
    QPushButton *closeBtn = new QPushButton("✕", ui->tabWidget);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setStyleSheet(
        "QPushButton { color: #AAAAAA; border: none; background: transparent; "
        "font-size: 12px; font-weight: bold; } QPushButton:hover { color: "
        "#FFFF55; }");
    tabBar->setTabButton(index, QTabBar::RightSide, closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, [this, cw]() {
      int idx = ui->tabWidget->indexOf(cw);
      if (idx != -1)
        onTabCloseRequested(idx);
    });
  }

  ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::onAddJoyNodeClicked() {
  ui->stackedWidget->setCurrentIndex(1); // Switch to tabs view
  int targetIndex = ui->tabWidget->count() - 1;
  if (targetIndex < 0)
    targetIndex = 0;

  int nextNum = 1;
  while (true) {
    bool numUsed = false;
    QString nameToCheck = QString("JOYNODE %1").arg(nextNum);
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
      if (ui->tabWidget->tabText(i) == nameToCheck) {
        numUsed = true;
        break;
      }
    }
    if (!numUsed)
      break;
    nextNum++;
  }

  JoyNodeWidget *jw = new JoyNodeWidget(this);
  int index = ui->tabWidget->insertTab(targetIndex, jw,
                                       QString("JOYNODE %1").arg(nextNum));

  connect(rosWorker_, &RosWorker::joystickDataUpdated, jw,
          &JoyNodeWidget::updateJoystickData);

  QTabBar *tabBar = ui->tabWidget->findChild<QTabBar *>();
  if (tabBar) {
    QPushButton *closeBtn = new QPushButton("✕", ui->tabWidget);
    closeBtn->setFixedSize(16, 16);
    closeBtn->setStyleSheet(
        "QPushButton { color: #AAAAAA; border: none; background: transparent; "
        "font-size: 12px; font-weight: bold; } QPushButton:hover { color: "
        "#FFFF55; }");
    tabBar->setTabButton(index, QTabBar::RightSide, closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, [this, jw]() {
      int idx = ui->tabWidget->indexOf(jw);
      if (idx != -1)
        onTabCloseRequested(idx);
    });
  }

  ui->tabWidget->setCurrentIndex(index);
}

void MainWindow::onTabCloseRequested(int index) {
  if (index == ui->tabWidget->count() - 1)
    return; // Prevent closing the dummy tab
  QWidget *widget = ui->tabWidget->widget(index);
  ui->tabWidget->removeTab(index);
  widget->deleteLater();

  if (ui->tabWidget->count() <= 1) {       // Only the dummy left
    ui->stackedWidget->setCurrentIndex(0); // Revert to empty state
  }
}
