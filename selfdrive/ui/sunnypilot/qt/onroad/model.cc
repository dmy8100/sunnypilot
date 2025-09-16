/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/onroad/model.h"


void ModelRendererSP::update_model(const cereal::ModelDataV2::Reader &model, const cereal::RadarState::LeadData::Reader &lead) {
  ModelRenderer::update_model(model, lead);
  const auto &model_position = model.getPosition();
  const auto &lane_lines = model.getLaneLines();
  float max_distance = std::clamp(*(model_position.getX().end() - 1), MIN_DRAW_DISTANCE, MAX_DRAW_DISTANCE);
  int max_idx = get_path_length_idx(lane_lines[0], max_distance);
  // update blindspot vertices
  float max_distance_barrier = 100;
  int max_idx_barrier = std::min(max_idx, get_path_length_idx(lane_lines[0], max_distance_barrier));
  mapLineToPolygon(model.getLaneLines()[1], 0.2, -0.05, &left_blindspot_vertices, max_idx_barrier);
  mapLineToPolygon(model.getLaneLines()[2], 0.2, -0.05, &right_blindspot_vertices, max_idx_barrier);
}

void ModelRendererSP::drawPath(QPainter &painter, const cereal::ModelDataV2::Reader &model, const QRect &surface_rect) {
  auto *s = uiState();
  auto &sm = *(s->sm);
  bool blindspot = Params().getBool("BlindSpot");

  if (blindspot) {
    bool left_blindspot = sm["carState"].getCarState().getLeftBlindspot();
    bool right_blindspot = sm["carState"].getCarState().getRightBlindspot();

    //painter.setBrush(QColor::fromRgbF(1.0, 0.0, 0.0, 0.4));  // Red with alpha for blind spot

    if (left_blindspot && !left_blindspot_vertices.isEmpty()) {
      QLinearGradient gradient(0, 0, surface_rect.width(), 0); // Horizontal gradient from left to right
      gradient.setColorAt(0.0, QColor(255, 165, 0, 102)); // Orange with alpha
      gradient.setColorAt(1.0, QColor(255, 255, 0, 102)); // Yellow with alpha
      painter.setBrush(gradient);
      painter.drawPolygon(left_blindspot_vertices);
    }

    if (right_blindspot && !right_blindspot_vertices.isEmpty()) {
      QLinearGradient gradient(surface_rect.width(), 0, 0, 0); // Horizontal gradient from right to left
      gradient.setColorAt(0.0, QColor(255, 165, 0, 102)); // Orange with alpha
      gradient.setColorAt(1.0, QColor(255, 255, 0, 102)); // Yellow with alpha
      painter.setBrush(gradient);
      painter.drawPolygon(right_blindspot_vertices);
    }
  }

  bool rainbow = Params().getBool("RainbowMode");
  //float v_ego = sm["carState"].getCarState().getVEgo();

  if (rainbow) {
    // Simple time-based animation
    float time_offset = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0f;

    // simple linear gradient from bottom to top
    QLinearGradient bg(0, surface_rect.height(), 0, 0);

    // evenly spaced colors across the spectrum
    // The animation shifts the entire spectrum smoothly
    float animation_speed = 40.0f; // speed vroom vroom
    float hue_offset = fmod(time_offset * animation_speed, 360.0f);

    // 6-8 color stops for smooth transitions more color makes it laggy
    const int num_stops = 7;
    for (int i = 0; i < num_stops; i++) {
      float position = static_cast<float>(i) / (num_stops - 1);

      float hue = fmod(hue_offset + position * 360.0f, 360.0f);
      float saturation = 0.9f;
      float lightness = 0.6f;

      // Alpha fades out towards the far end of the path
      float alpha = 0.8f * (1.0f - position * 0.3f);

      QColor color = QColor::fromHslF(hue / 360.0f, saturation, lightness, alpha);
      bg.setColorAt(position, color);
    }

    painter.setBrush(bg);
    painter.drawPolygon(track_vertices);
  }
  // Always draw lead status regardless of RainbowMode
  drawLeadStatus(painter, surface_rect.height(), surface_rect.width());
  // Normal path rendering
  ModelRenderer::drawPath(painter, model, surface_rect.height());
}

void ModelRendererSP::drawLeadStatus(QPainter &painter, int height, int width) {
  auto *s = uiState();
  auto &sm = *(s->sm);

  bool longitudinal_control = sm["carParams"].getCarParams().getOpenpilotLongitudinalControl();
  if (!longitudinal_control) {
    lead_status_alpha = std::max(0.0f, lead_status_alpha - 0.05f);
    return;
  }

  if (!sm.alive("radarState")) {
    lead_status_alpha = std::max(0.0f, lead_status_alpha - 0.05f);
    return;
  }

  const auto &radar_state = sm["radarState"].getRadarState();
  const auto &lead_one = radar_state.getLeadOne();
  const auto &lead_two = radar_state.getLeadTwo();

  bool has_lead_one = lead_one.getStatus();
  bool has_lead_two = lead_two.getStatus();

  if (!has_lead_one && !has_lead_two) {
    lead_status_alpha = std::max(0.0f, lead_status_alpha - 0.05f);
    if (lead_status_alpha <= 0.0f) return;
  } else {
    lead_status_alpha = std::min(1.0f, lead_status_alpha + 0.1f);
  }

  if (has_lead_one) {
    drawLeadStatusAtPosition(painter, lead_one, lead_vertices[0], height, width, "L1");
  }

  if (has_lead_two && std::abs(lead_one.getDRel() - lead_two.getDRel()) > 3.0) {
    drawLeadStatusAtPosition(painter, lead_two, lead_vertices[1], height, width, "L2");
  }
}

void ModelRendererSP::drawLeadStatusAtPosition(QPainter &painter,
                                               const cereal::RadarState::LeadData::Reader &lead_data,
                                               const QPointF &chevron_pos,
                                               int height, int width,
                                               const QString &label) {
  float d_rel = lead_data.getDRel();
  float v_rel = lead_data.getVRel();
  auto *s = uiState();
  auto &sm = *(s->sm);
  float v_ego = sm["carState"].getCarState().getVEgo();

  int chevron_data = std::atoi(Params().get("ChevronInfo").c_str());
  float sz = std::clamp((25 * 30) / (d_rel / 3 + 30), 15.0f, 30.0f) * 2.35;

    QFont content_font = painter.font();
    content_font.setPixelSize(65); // 增大字体到65
    content_font.setBold(true);
    painter.setFont(content_font);

    bool is_metric = s->scene.is_metric;

    // 分离速度信息和其他信息
    QStringList top_text_lines;    // 显示在chevron上方的文本（速度）
    QStringList bottom_text_lines; // 显示在chevron下方的文本（距离和TTC）

    const int chevron_all = 4;

    // 速度显示 - 放在上方，不显示单位
    if (chevron_data == 2 || chevron_data == chevron_all) {
        float multiplier = is_metric ? static_cast<float>(MS_TO_KPH) : static_cast<float>(MS_TO_MPH);
        float val = std::max(0.0f, (v_rel + v_ego) * multiplier);
        // 移除单位，只显示数值
        top_text_lines.append(QString::number(val, 'f', 0));
    }

    // 距离显示 - 放在下方
    if (chevron_data == 1 || chevron_data == chevron_all) {
        float val = std::max(0.0f, d_rel);
        QString unit = is_metric ? "m" : "ft";
        if (!is_metric) val *= 3.28084f;
        bottom_text_lines.append(QString::number(val, 'f', 0) + " " + unit);
    }

    // 时间到接触显示 - 放在下方
    const bool SHOW_TTC = false; // 设置为false不显示碰撞时间，true则显示
    if (SHOW_TTC && (chevron_data == 3 || chevron_data == chevron_all)) {
        float val = (d_rel > 0 && v_ego > 0) ? std::max(0.0f, d_rel / v_ego) : 0.0f;
        QString ttc = (val > 0 && val < 200) ? QString::number(val, 'f', 1) + "s" : "---";
        bottom_text_lines.append(ttc);
    }

    // 如果没有内容需要显示，直接返回
    if (top_text_lines.isEmpty() && bottom_text_lines.isEmpty()) return;

    QFontMetrics fm(content_font);
    float margin = 20.0f;
    float line_height = 65.0f; // 增大行高以适应更大的字体

    // 计算上方文本区域
    float top_text_width = 120.0f;
    for (const QString &line : top_text_lines) {
        top_text_width = std::max(top_text_width, fm.horizontalAdvance(line) + 20.0f);
    }
    top_text_width = std::min(top_text_width, 250.0f);


    // 计算下方文本区域
    float bottom_text_width = 120.0f;
    for (const QString &line : bottom_text_lines) {
        bottom_text_width = std::max(bottom_text_width, fm.horizontalAdvance(line) + 20.0f);
    }
    bottom_text_width = std::min(bottom_text_width, 250.0f);
    float bottom_total_height = bottom_text_lines.size() * line_height;

    // 计算上方文本位置 - 进一步降低位置（增加与chevron的距离）
    float top_text_y = chevron_pos.y() - sz - 35; // 从5改为30，进一步降低位置
    top_text_y = std::max(margin, top_text_y);
    float top_text_x = chevron_pos.x() - top_text_width / 2;
    top_text_x = std::clamp(top_text_x, margin, (float)width - top_text_width - margin);

    // 计算下方文本位置
    float bottom_text_y = chevron_pos.y() + sz + 15;
    if (bottom_text_y + bottom_total_height > height - margin) {
        bottom_text_y = chevron_pos.y() - sz - 15 - bottom_total_height;
        bottom_text_y = std::max(margin, bottom_text_y);
    }
    float bottom_text_x = chevron_pos.x() - bottom_text_width / 2;
    bottom_text_x = std::clamp(bottom_text_x, margin, (float)width - bottom_text_width - margin);

  QPoint shadow_offset(2, 2);

    // 绘制上方文本（速度）
    for (int i = 0; i < top_text_lines.size(); ++i) {
        float y = top_text_y + (i * line_height);
        if (y + line_height > height - margin) break;

        QRect rect(top_text_x, y, top_text_width, line_height);

        // 绘制阴影
        painter.setPen(QColor(0, 0, 0, (int)(200 * lead_status_alpha)));
        painter.drawText(rect.translated(shadow_offset), Qt::AlignCenter, top_text_lines[i]);

        // 速度信息使用与距离信息相同的颜色逻辑
        QColor text_color = QColor(255, 255, 255, (int)(255 * lead_status_alpha));
        // 根据距离改变颜色
        if (d_rel < 20.0f) {
            text_color = QColor(255, 80, 80, (int)(255 * lead_status_alpha));
        } else if (d_rel < 40.0f) {
            text_color = QColor(255, 200, 80, (int)(255 * lead_status_alpha));
        } else {
            text_color = QColor(80, 255, 120, (int)(255 * lead_status_alpha));
        }

        painter.setPen(text_color);
        painter.drawText(rect, Qt::AlignCenter, top_text_lines[i]);
    }

    // 绘制下方文本（距离和TTC）
    for (int i = 0; i < bottom_text_lines.size(); ++i) {
        float y = bottom_text_y + (i * line_height);
        if (y + line_height > height - margin) break;

        QRect rect(bottom_text_x, y, bottom_text_width, line_height);

        // 绘制阴影
        painter.setPen(QColor(0, 0, 0, (int)(200 * lead_status_alpha)));
        painter.drawText(rect.translated(shadow_offset), Qt::AlignCenter, bottom_text_lines[i]);

        QColor text_color = QColor(255, 255, 255, (int)(255 * lead_status_alpha));
        // 只有距离信息才根据距离改变颜色
        if (bottom_text_lines[i].contains("m") || bottom_text_lines[i].contains("ft")) {
            if (d_rel < 20.0f) {
                text_color = QColor(255, 80, 80, (int)(255 * lead_status_alpha));
            } else if (d_rel < 40.0f) {
                text_color = QColor(255, 200, 80, (int)(255 * lead_status_alpha));
            } else {
                text_color = QColor(80, 255, 120, (int)(255 * lead_status_alpha));
            }
        }

        painter.setPen(text_color);
        painter.drawText(rect, Qt::AlignCenter, bottom_text_lines[i]);
    }

  painter.setPen(Qt::NoPen);
}