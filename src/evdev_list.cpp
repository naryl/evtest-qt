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

#include "evdev_list.hpp"

#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <stdexcept>
#include <string.h>
#include <fnmatch.h>
#include <sstream>

std::vector<std::string>
EvdevList::scan(const std::string& evdev_directory)
{
  DIR* dirp = opendir(evdev_directory.c_str());
  if (!dirp)
  {
    throw std::runtime_error(evdev_directory + ": " + strerror(errno));
  }
  else
  {
    std::vector<std::string> devices;

    struct dirent* dentry;
    while(dentry = readdir(dirp))
    {
      if (fnmatch("event[0-9]", dentry->d_name, FNM_PATHNAME) == 0 ||
          fnmatch("event[0-9][0-9]", dentry->d_name, FNM_PATHNAME) == 0)
      {
        std::ostringstream str;
        str << evdev_directory << "/" << dentry->d_name;
        devices.push_back(str.str());
      }
    }
    closedir(dirp);

    std::sort(devices.begin(), devices.end(),
              [](auto const& lhs, auto const& rhs)
              {
                if (lhs.size() < rhs.size())
                {
                  return true;
                }
                else
                {
                  return lhs < rhs;
                }
              });

    return devices;
  }
}

/* EOF */
