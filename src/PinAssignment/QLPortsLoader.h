#pragma once

#include "PortsLoader.h"

namespace FOEDAG {

class QLPortsLoader : public PortsLoader {
 public:
  QLPortsLoader(PortsModel *model, QObject *parent = nullptr);
  std::pair<bool, QString> load(const QString &file) override final;

private:
  void addPort(IOPortGroup& group, const QString& portName, const QString& portDirection);
};

}  // namespace FOEDAG
