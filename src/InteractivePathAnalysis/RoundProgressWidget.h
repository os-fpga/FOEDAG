/**
  * @file NCriticalPathReportParser.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-05-14
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QEasingCurve>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

namespace FOEDAG {

class RoundProgressWidget : public QWidget {
  Q_OBJECT

  const int ANIMATION_INTERVAL_MS = 20;
  const int ROTATION_DEGREES_MAX = 360;
  const qreal ANIM_PROGRESS_STEP_NORM = ANIMATION_INTERVAL_MS * 0.001;
  const qreal ANIM_PROGRESS_START_NORM = 0.0;
  const qreal ANIM_PROGRESS_END_NORM = 1.0;

 public:
  RoundProgressWidget(int size, QWidget* parent = nullptr);
  ~RoundProgressWidget() = default;

 protected:
  void showEvent(QShowEvent*) override final;
  void hideEvent(QHideEvent*) override final;
  void paintEvent(QPaintEvent*) override final;

 private:
  QTimer m_timer;
  QPixmap m_pixmap;
  qreal m_animProgressNorm = 0.0;

  QEasingCurve m_animCurve;

  void resetAnimationProgress();
};

}  // namespace FOEDAG
