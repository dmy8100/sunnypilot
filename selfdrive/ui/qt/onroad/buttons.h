#pragma once

#include <QPushButton>

#ifdef SUNNYPILOT
#include "selfdrive/ui/sunnypilot/ui.h"
#else
#include "selfdrive/ui/ui.h"
#endif

const int btn_size = 192;
const int longitudinal_btn_size = 200; // 纵向控制模式切换按钮的独立尺寸
const int img_size = (btn_size / 9) * 8;
const int longitudinal_img_size = (btn_size / 8) * 8;

class ExperimentalButton : public QPushButton {
  Q_OBJECT

public:
  explicit ExperimentalButton(QWidget *parent = 0);
  virtual void updateState(const UIState &s);

private:
  void paintEvent(QPaintEvent *event) override;
  void changeMode();

  Params params;

protected:
  virtual void drawButton(QPainter &p);

  QPixmap engage_img;
  QPixmap experimental_img;
  bool experimental_mode;
  bool engageable;
};

class LongitudinalPersonalityButton : public QPushButton {
  Q_OBJECT

public:
  explicit LongitudinalPersonalityButton(QWidget *parent = 0);
  void updateState(const UIState &s);

private:
  void paintEvent(QPaintEvent *event) override;
  void changeMode();

  Params params;
  int current_personality; // 0: Aggressive, 1: Standard, 2: Relaxed

protected:
  void drawButton(QPainter &p);

  QPixmap aggressive_img;
  QPixmap standard_img;
  QPixmap relaxed_img;
};

void drawIcon(QPainter &p, const QPoint &center, const QPixmap &img, const QBrush &bg, float opacity);
