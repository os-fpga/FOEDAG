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
#pragma once

#include <QObject>

#include "Programmer/ProgrammerGuiInterface.h"
#include "ProgrammerGuiCommon.h"
#include "Utils/sequential_map.h"

namespace FOEDAG {

struct DeviceBitstream {
  std::string bitstream{};
  std::string flashBitstream{};
};

class ProgrammerGuiIntegration : public QObject, public ProgrammerGuiInterface {
  Q_OBJECT

 public:
  explicit ProgrammerGuiIntegration(QObject *parent = nullptr);
  const sequential_map<ProgrammerCable, std::vector<ProgrammerDevice>>
      &devices() const;
  void Cables(const std::vector<Cable> &cables) override;
  void Devices(const Cable &cable, const std::vector<Device> &devices) override;
  void Progress(const std::string &progress) override;
  void ProgramFpga(const Cable &cable, const Device &device,
                   const std::string &file) override;
  void ProgramOtp(const Cable &cable, const Device &device,
                  const std::string &file) override;
  void Flash(const Cable &cable, const Device &device,
             const std::string &file) override;
  void Status(const Cable &cable, const Device &device, int status) override;
  std::atomic_bool &Stop() override;

  std::string File(const ProgrammerDevice &dev, bool flash) const;
  void StopLastProcess();

 signals:
  void progress(const DeviceEntity &, const std::string &progress);
  void autoDetect();
  void programStarted(const DeviceEntity &);
  void status(const DeviceEntity &, int status);

 private:
  sequential_map<ProgrammerCable, std::vector<ProgrammerDevice>> m_devices;
  std::pair<ProgrammerCable, ProgrammerDevice> m_current;
  std::map<ProgrammerDevice, DeviceBitstream> m_files;
  Type m_type{};
  std::atomic_bool m_stop;
};

}  // namespace FOEDAG
