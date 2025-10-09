#pragma once

#include "selfdrive/ui/qt/onroad/alerts.h"

#ifdef SUNNYPILOT
#include "selfdrive/ui/sunnypilot/qt/onroad/annotated_camera.h"
#include "selfdrive/ui/sunnypilot/qt/onroad/alerts.h"
#define UIState UIStateSP
#define AnnotatedCameraWidget AnnotatedCameraWidgetSP
#define OnroadAlerts OnroadAlertsSP
#else
#include "selfdrive/ui/qt/onroad/annotated_camera.h"
#endif

// 前向声明
class LongitudinalPersonalityButton;

class OnroadWindow : public QWidget {
  Q_OBJECT

public:
  OnroadWindow(QWidget* parent = 0);

protected:
  void paintEvent(QPaintEvent *event);
  OnroadAlerts *alerts;
  AnnotatedCameraWidget *nvg;
  QColor bg = bg_colors[STATUS_DISENGAGED];
  QHBoxLayout* split;
  
  // 添加LongitudinalPersonalityButton成员变量
  LongitudinalPersonalityButton *longitudinal_personality_button;

  // Indicator state variables
  bool dp_indicator_show_left = false;
  bool dp_indicator_show_right = false;
  bool dp_indicator_show_left_prev = false;
  bool dp_indicator_show_right_prev = false;
  int dp_indicator_count_left = 0;
  int dp_indicator_count_right = 0;
  QColor dp_indicator_color_left;
  QColor dp_indicator_color_right;

  // Indicator constants
  static const QColor DP_INDICATOR_COLOR_BSM;
  static const QColor DP_INDICATOR_COLOR_BLINKER;
  static const int DP_INDICATOR_BLINK_RATE_FAST = 10;
  static const int DP_INDICATOR_BLINK_RATE_STD = 20;

  void updateDpIndicatorStates(const UIState &s);
  void updateDpIndicatorSideState(bool blinker_state, bool bsm_state, bool &show, bool &show_prev, int &count, QColor &color);

protected slots:
  virtual void offroadTransition(bool offroad);
  virtual void updateState(const UIState &s);
};
