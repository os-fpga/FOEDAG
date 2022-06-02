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

#include "Log.h"

namespace FOEDAG {

std::ostream& formatDateTime(std::ostream& out, const tm& t, const char* fmt) {
  const std::time_put<char>& dateWriter =
      std::use_facet<std::time_put<char> >(out.getloc());
  int n = strlen(fmt);
  dateWriter.put(out, out, ' ', &t, fmt, fmt + n);
  return out;
}

std::string dateTimeToString(const tm& t, const char* format) {
  std::stringstream s;
  formatDateTime(s, t, format);
  return s.str();
}

tm now() {
  time_t now = time(nullptr);
  return *localtime(&now);
}
}  // namespace FOEDAG
