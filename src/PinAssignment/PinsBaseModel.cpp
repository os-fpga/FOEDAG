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
#include "PinsBaseModel.h"

namespace FOEDAG {

PinsBaseModel::PinsBaseModel() {}

QStringList PinsBaseModel::banks() const { return {"Bank 0", "Bank 1"}; }

QVector<IOPort> PinsBaseModel::ioPorts() const {
  QVector<IOPort> ports;
  ports.append({"IN0", "INPUT", {}, {}});
  ports.append({"IN1", "INPUT", {}, {}});
  ports.append({"OUT", "OUTPUT", {}, {}});
  return ports;
}

QVector<PinBank> PinsBaseModel::packagePins() const {
  QVector<PinBank> banks;
  banks.append({"System", {{"RST_N", {}}, {"XIN", {}}}});
  banks.append({"JTAG", {{"JTAG_TDI", {}}, {"JTAG_TDO", {}}}});
  banks.append({"GPIO", {{"GPIO_A_0", {}}, {"GPIO_A_1", {}}}});
  banks.append({"GBe", {{"MDIO_MDCÂ", {}}, {"MDIO_DATAÂ", {}}}});
  banks.append({"USB", {{"USB_DPÂ", {}}, {"USB_DNÂ", {}}}});
  banks.append({"I2C CLK", {{"I2C_SCLÂ", {}}}});
  banks.append({"SPI CLK", {{"SPI_SCLKÂ", {}}}});
  banks.append({"GPT", {{"GPT_RTC", {}}}});
  banks.append({"DDR", {{"PAD_MEM_DQS_N[0]", {}}, {"PAD_MEM_DQS_N[1]", {}}}});
  return banks;
}
}  // namespace FOEDAG
