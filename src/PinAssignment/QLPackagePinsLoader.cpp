#include "QLPackagePinsLoader.h"

#include <Utils/QtUtils.h>

#include <QSet>

namespace FOEDAG {

QLPackagePinsLoader::QLPackagePinsLoader(PackagePinsModel *model, QObject *parent)
    : PackagePinsLoader(model, parent) {}

std::pair<bool, QString> QLPackagePinsLoader::load(const QString &fileName) {
  const auto &[success, content] = getFileContent(fileName);
  if (!success) return std::make_pair(success, content);

  InternalPins &internalPins = m_model->internalPinsRef();

  QStringList lines = QtUtils::StringSplit(content, '\n');
  parseHeader(lines.takeFirst());
  PackagePinGroup group{};
  QSet<QString> uniquePins;
  for (const auto &line : lines) {
    QStringList data = line.split(",");
    if (!data.first().isEmpty()) {
      if (!group.name.isEmpty() && (group.name != data.first())) {
        if (m_model->userGroups().contains(group.name)) m_model->append(group);
        group.pinData.clear();
        uniquePins.clear();
      }
      group.name = data.first();
    }

    QStringList dataMod;
    for (int i=0; i<=InternalPinName; ++i) {
      dataMod.append("");
    }

    if (data.size() >= BALLNAME_COLUMN) {
      dataMod[PinName] = data.at(BALLNAME_COLUMN);
      dataMod[BallName] = data.at(BALLNAME_COLUMN);
      dataMod[BallId] = data.at(BALLNAME_COLUMN);
    }
    if (data.size() >= INTERNAL_PINNAME_COLUMN) {
      dataMod[InternalPinName] = data.at(INTERNAL_PINNAME_COLUMN);
    }

    if (uniquePins.contains(data.at(BALLNAME_COLUMN))) continue;
    uniquePins.insert(data.at(BALLNAME_COLUMN));

    group.pinData.append({dataMod});
  }
  if (m_model->userGroups().contains(group.name))
    m_model->append(group);  // append last
  m_model->initListModel();

  return std::make_pair(true, QString());
}

}  // namespace FOEDAG
