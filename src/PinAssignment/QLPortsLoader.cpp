#include "QLPortsLoader.h"

#include <QFile>

namespace FOEDAG {

QLPortsLoader::QLPortsLoader(PortsModel *model, QObject *parent)
    : PortsLoader(model, parent) {}

std::pair<bool, QString> QLPortsLoader::load(const QString& filePath) {
  if (!filePath.endsWith(".blif")) {
    return std::make_pair(false, QString("QLPortsLoader: %1 is not a blif format").arg(filePath));
  }
  QFile file{filePath};
  if (!file.open(QFile::ReadOnly)) {
    return std::make_pair(false, QString("QLPortsLoader: Unable to open %1 file for reading").arg(filePath));
  }

  bool got_inputs = false;
  bool got_outputs = false;

  QList<QString> inputs;
  QList<QString> outputs;
  while (!file.atEnd()) {
    QString line = file.readLine();
    if (line.startsWith(".inputs ")) {
      line = line.replace(".inputs ", "").trimmed();
      inputs = line.split(" ");
      got_inputs = true;
    }
    if (line.startsWith(".outputs ")) {
      line = line.replace(".outputs ", "").trimmed();
      outputs = line.split(" ");
      got_outputs = true;
    }

    if (got_inputs && got_outputs) {
      break;
    }
  }

  IOPortGroup group;
  for (const QString& input: inputs) {
    addPort(group, input, "Input");
  }
  for (const QString& output: outputs) {
    addPort(group, output, "Output");
  }
  m_model->append(group);
  m_model->initListModel();

  return std::make_pair(true, QString());
}

void QLPortsLoader::addPort(IOPortGroup& group, const QString& portName, const QString& portDirection)
{
  const int msb = 0; // not used
  const int lsb = 0; // not used

  IOPort ioport{portName,
          QString(portDirection),
          QString(),
          QString("LOGIC"),
          QString("Msb: %1, lsb: %2")
              .arg(QString::number(msb), QString::number(lsb)),
          (msb != lsb),
          {}};

  group.ports.append(ioport);
}

}  // namespace FOEDAG
