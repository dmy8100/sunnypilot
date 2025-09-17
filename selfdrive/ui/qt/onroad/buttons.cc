#include "selfdrive/ui/qt/onroad/buttons.h"

#include <QPainter>
#include <string>

#include "selfdrive/ui/qt/util.h"

void drawIcon(QPainter &p, const QPoint &center, const QPixmap &img, const QBrush &bg, float opacity) {
  p.setRenderHint(QPainter::Antialiasing);
  p.setOpacity(1.0);  // bg dictates opacity of ellipse
  p.setPen(Qt::NoPen);
  p.setBrush(bg);
  p.drawEllipse(center, btn_size / 2, btn_size / 2);
  p.setOpacity(opacity);
  p.drawPixmap(center - QPoint(img.width() / 2, img.height() / 2), img);
  p.setOpacity(1.0);
}

// ExperimentalButton
ExperimentalButton::ExperimentalButton(QWidget *parent) : experimental_mode(false), engageable(false), QPushButton(parent) {
  setFixedSize(btn_size, btn_size);

  engage_img = loadPixmap("../assets/icons/chffr_wheel.png", {longitudinal_img_size, longitudinal_img_size});
  experimental_img = loadPixmap("../assets/icons/experimental.svg", {img_size, img_size});
  QObject::connect(this, &QPushButton::clicked, this, &ExperimentalButton::changeMode);
}

void ExperimentalButton::changeMode() {
  const auto cp = (*uiState()->sm)["carParams"].getCarParams();
  bool can_change = hasLongitudinalControl(cp) && params.getBool("ExperimentalModeConfirmed");
  if (can_change) {
    params.putBool("ExperimentalMode", !experimental_mode);
  }
}

void ExperimentalButton::updateState(const UIState &s) {
  const auto cs = (*s.sm)["selfdriveState"].getSelfdriveState();
  bool eng = cs.getEngageable() || cs.getEnabled();
  if ((cs.getExperimentalMode() != experimental_mode) || (eng != engageable)) {
    engageable = eng;
    experimental_mode = cs.getExperimentalMode();
    update();
  }
}

void ExperimentalButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  drawButton(p);
}

void ExperimentalButton::drawButton(QPainter &p) {
  QPixmap img = experimental_mode ? experimental_img : engage_img;
  drawIcon(p, QPoint(btn_size / 2, btn_size / 2), img, QColor(0, 0, 0, 0), (isDown() || !engageable) ? 0.6 : 1.0);
}

// LongitudinalPersonalityButton
LongitudinalPersonalityButton::LongitudinalPersonalityButton(QWidget *parent) : current_personality(1), QPushButton(parent) {
  setFixedSize(btn_size, btn_size);

  aggressive_img = loadPixmap("../assets/distance_icons/aggressive.png", {longitudinal_img_size, longitudinal_img_size});
  standard_img = loadPixmap("../assets/distance_icons/standard.png", {longitudinal_img_size, longitudinal_img_size});
  relaxed_img = loadPixmap("../assets/distance_icons/relaxed.png", {longitudinal_img_size, longitudinal_img_size});

  QObject::connect(this, &QPushButton::clicked, this, &LongitudinalPersonalityButton::changeMode);
}

void LongitudinalPersonalityButton::changeMode() {
  current_personality = (current_personality + 1) % 3;
  // 使用put方法存储字符串值
  params.put("LongitudinalPersonality", std::to_string(current_personality));
}

void LongitudinalPersonalityButton::updateState(const UIState &s) {
  // 使用get方法获取字符串值并转换为整数
  std::string personality_str = params.get("LongitudinalPersonality");
  if (!personality_str.empty()) {
    try {
      int personality = std::stoi(personality_str);
      if (personality != current_personality) {
        current_personality = personality;
        update();
      }
    } catch (const std::exception& e) {
      // 如果转换失败，使用默认值
      current_personality = 1;
      params.put("LongitudinalPersonality", "1");
    }
  } else {
    // 如果参数不存在，设置默认值
    current_personality = 1;
    params.put("LongitudinalPersonality", "1");
  }
}

void LongitudinalPersonalityButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  drawButton(p);
}

void LongitudinalPersonalityButton::drawButton(QPainter &p) {
  QPixmap img;
  switch (current_personality) {
    case 0: img = aggressive_img; break;
    case 1: img = standard_img; break;
    case 2: img = relaxed_img; break;
    default: img = standard_img; break;
  }
  drawIcon(p, QPoint(btn_size / 2, btn_size / 2 - 30), img, QColor(0, 0, 0, 0), isDown() ? 0.6 : 1.0);
}
