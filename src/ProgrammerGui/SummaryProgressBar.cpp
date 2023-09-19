/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SummaryProgressBar.h"

#include <QProgressBar>
#include <cmath>

namespace FOEDAG {

SummaryProgressBar::SummaryProgressBar(QObject *parent)
    : QObject(parent), m_progressBar(new QProgressBar) {}

QProgressBar *SummaryProgressBar::progressBar() { return m_progressBar; }

void SummaryProgressBar::clear() {
  m_bars.clear();
  updateMainProgress();
}

void SummaryProgressBar::AddProgressBar(QProgressBar *progressBar) {
  m_bars.insert(progressBar, 0);
  connect(progressBar, &QProgressBar::valueChanged, this,
          [this, progressBar](int val) {
            m_bars[progressBar] = (static_cast<double>(val) / 100.0) *
                                  (100.0 / static_cast<double>(m_bars.size()));
            updateMainProgress();
          });
}

void SummaryProgressBar::updateMainProgress() {
  double sum{0};
  for (const auto &val : qAsConst(m_bars)) sum += val;
  m_progressBar->setValue(std::round(sum));
}

}  // namespace FOEDAG
