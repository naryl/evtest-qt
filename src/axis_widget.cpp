// evtest-qt - A graphical joystick tester
// Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "axis_widget.hpp"

#include <QPainter>
#include <iostream>

#include "evdev_enum.hpp"
#include "evdev_state.hpp"

bool AxisWidget::is_tested() const
{
    return m_saw_min && m_saw_max;
}

AxisWidget::AxisWidget(uint16_t code, int min, int max, QWidget* parent_) :
  QWidget(parent_),
  m_code(code),
  m_min(min),
  m_max(max),
  m_value(0),
  m_saw_min(false),
  m_saw_max(false)
{
  setToolTip(QString::fromStdString(evdev_abs_name(m_code)));
}

AxisWidget::~AxisWidget()
{
}

void
AxisWidget::set_axis_pos(int v)
{
  m_value = v;
  update();
}

void
AxisWidget::on_change(const EvdevState& state)
{
  int old_value = m_value;
  m_value = state.get_abs_value(m_code);
  if (old_value != m_value)
  {
    if (m_value <= m_min) {
      m_saw_min = true;
    }
    if (m_value >= m_max) {
      m_saw_max = true;
    }
    update();
  }
}

void
AxisWidget::paintEvent(QPaintEvent* ev)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  int value_pos = 0;
  int zero_pos = 0;

  if (m_max - m_min != 0)
  {
    value_pos = width() * (m_value - m_min) / (m_max - m_min);
    zero_pos = width() * (0 - m_min) / (m_max - m_min);
  }

  if (is_tested()) {
    // red rect
    painter.setPen(Qt::NoPen);
    painter.fillRect(0, 0, width(), height(), QColor(0, 255, 0));
  }
  else {
    // blue rect
    painter.setPen(Qt::NoPen);
    int l = std::min(value_pos, zero_pos);
    int r = std::max(value_pos, zero_pos);
    painter.fillRect(l, 0, r - l, height(), QColor(192, 192, 255));
  }

  // value line
  painter.setPen(QColor(0, 0, 0));
  painter.drawLine(value_pos, 0, value_pos, height());

  // box outline
  painter.setPen(QColor(0, 0, 0));
  painter.drawRect(0, 0, width(), height());
}

/* EOF */
