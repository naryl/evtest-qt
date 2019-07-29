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

#include "evtest_app.hpp"

#include <QTimer>
#include <iostream>

#include "util.hpp"
#include "evdev_widget.hpp"

void EvtestApp::display_message(QString message)
{
    QFont font("Arial", 16, QFont::Bold);
    m_ev_widget = util::make_unique<QLabel>(message);
    QLabel* label = static_cast<QLabel*>(m_ev_widget.get());
    label->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    label->setFont(font);
    m_vbox_layout.addWidget(m_ev_widget.get());
}

EvtestApp::EvtestApp() :
  m_window(),
  m_widget(),
  m_vbox_layout(&m_widget),
  m_ev_widget(),
  m_device(),
  m_state(),
  m_notifier(),
  m_tested(false),
  m_initialized_devices(false)
{
  //m_widget.setMinimumSize(400, 300);
  m_window.setCentralWidget(&m_widget);

  display_message("Please, plug in the device.");

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(refresh_device_list()));
  timer->start(1000);

  m_window.show();
}



EvdevInfo EvtestApp::device_info(QString device)
{
    auto dev_fp = EvdevDevice::open(device.toStdString());
    return dev_fp->read_evdev_info();
}

void
EvtestApp::refresh_device_list()
{
  std::vector<std::string> new_devices = EvdevList::scan("/dev/input");
  for(auto dev : new_devices)
  {
    try
    {
      EvdevInfo info = device_info(QString::fromStdString(dev));

      std::ostringstream str;
      str << dev.substr(11) << ": " << info.name;
    }
    catch(const std::exception& err)
    {
      std::cout << dev << ": " << err.what() << std::endl;

      std::ostringstream str;
      str << dev.substr(11) << ": " << "[error: " << err.what() << "]";
    }
    QCoreApplication::processEvents();
  }

  if (!m_initialized_devices) {
    m_devices = new_devices;
    m_initialized_devices = true;
    return;
  }

  std::vector<std::string> added_devices, removed_devices;
  std::set_difference(new_devices.begin(), new_devices.end(), m_devices.begin(), m_devices.end(),
          std::inserter(added_devices, added_devices.begin()));
  std::set_difference(m_devices.begin(), m_devices.end(), new_devices.begin(), new_devices.end(),
          std::inserter(removed_devices, removed_devices.begin()));
  m_devices = new_devices;
  for (auto dev : added_devices)
      on_added_device(QString::fromStdString(dev));
  for (auto dev : removed_devices)
      on_removed_device(QString::fromStdString(dev));
}

void
EvtestApp::select_device(const QString& device)
{
  m_tested = false;
  on_device_change(device.toStdString());
}

void
EvtestApp::on_data(EvdevDevice& device, EvdevState& state)
{
  if (m_tested)
      return;
  std::array<struct input_event, 128> ev;
  while(true)
  {
    ssize_t num_events = device.read_events(ev.data(), ev.size());
    if (num_events < 0)
    {
      std::cout << "error: " << num_events << ": " << strerror(errno) << std::endl;
      return;
    }
    else if (num_events == 0)
    {
      EvdevWidget *evdev = qobject_cast<EvdevWidget *>(m_ev_widget.get());
      if (evdev && evdev->all_tested()) {
        display_message("PASS\nPlease unplug the device.");
        m_tested = true;
      }
      return;
    }
    else
    {
      for(ssize_t i = 0; i < num_events; ++i)
      {
        state.update(ev[static_cast<size_t>(i)]);
      }
    }
  }
}

void
EvtestApp::on_shrink_action()
{
  m_window.resize(0, 0);
}

void
EvtestApp::on_notification(int fd)
{
  on_data(*m_device, *m_state);
}

void
EvtestApp::on_device_change(const std::string& filename)
{
  m_notifier.reset();
  m_state.reset();
  m_ev_widget.reset();

  try
  {
    m_device = EvdevDevice::open(filename);
    auto info = m_device->read_evdev_info();

    m_state = util::make_unique<EvdevState>(info);

    m_ev_widget = util::make_unique<EvdevWidget>(*m_state, info);
    m_vbox_layout.addWidget(m_ev_widget.get());

    m_notifier = util::make_unique<QSocketNotifier>(m_device->get_fd(), QSocketNotifier::Read);

    QObject::connect(m_notifier.get(), SIGNAL(activated(int)),
                     this, SLOT(on_notification(int)));

    QTimer::singleShot(0, this, SIGNAL(on_shrink_action()));
  }
  catch(const std::exception& err)
  {
    std::cout << filename << ": " << err.what() << std::endl;
    m_ev_widget = util::make_unique<QLabel>(err.what());
    m_vbox_layout.addWidget(m_ev_widget.get());
  }
}

void EvtestApp::on_added_device(const QString &device)
{
  std::cout << "Added device:" << device.toStdString() << std::endl;
  select_device(device);
}

void EvtestApp::on_removed_device(const QString &device)
{
  std::cout << "Removed device:" << device.toStdString() << std::endl;
  display_message("Please, plug in the device.");

}

/* EOF */
