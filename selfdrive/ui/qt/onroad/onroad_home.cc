#include "selfdrive/ui/qt/onroad/onroad_home.h"

#include <QPainter>
#include <QStackedLayout>

#include "selfdrive/ui/qt/util.h"

OnroadWindow::OnroadWindow(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, this);
    split->insertWidget(0, arCam);
  }

  stacked_layout->addWidget(split_wrapper);

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // 添加LongitudinalPersonalityButton
  QWidget *button_container = new QWidget(this);
  //button_container->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  QHBoxLayout *button_layout = new QHBoxLayout(button_container);
  button_layout->setContentsMargins(UI_BORDER_SIZE, UI_BORDER_SIZE, UI_BORDER_SIZE, UI_BORDER_SIZE);
  button_layout->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
  
  longitudinal_personality_button = new LongitudinalPersonalityButton(button_container);
  button_layout->addWidget(longitudinal_personality_button);
  
  stacked_layout->addWidget(button_container);

  // setup stacking order
  alerts->raise();
  button_container->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);

  // We handle the connection of the signals on the derived class
#ifndef SUNNYPILOT
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);
#endif
}

void OnroadWindow::updateState(const UIState &s) {
  if (!s.scene.started) {
    return;
  }

  alerts->updateState(s);
  nvg->updateState(s);
  
  // 更新LongitudinalPersonalityButton状态
  if (longitudinal_personality_button) {
    longitudinal_personality_button->updateState(s);
  }

  QColor bgColor = bg_colors[s.status];
  if (bg != bgColor) {
    // repaint border
    bg = bgColor;
    update();
  }
}

void OnroadWindow::offroadTransition(bool offroad) {
  alerts->clear();
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), QColor(bg.red(), bg.green(), bg.blue(), 255));
}
