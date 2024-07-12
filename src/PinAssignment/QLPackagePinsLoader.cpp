#include "QLPackagePinsLoader.h"

#include <Utils/QtUtils.h>

#include <QSet>

namespace FOEDAG {

QLPackagePinsLoader::QLPackagePinsLoader(PackagePinsModel *model, QObject *parent)
    : PackagePinsLoader(model, parent) {
}

void QLPackagePinsLoader::initHeader()
{
  if (!m_model) {
    return;
  }

  if (!m_model->header().empty()) {
    return;
  }

  bool visible = true;

  int id = 0;

  m_model->appendHeaderData(HeaderData{"Ref clock", "Clock name referenced by pin", id++, true});
  m_model->appendHeaderData(HeaderData{"Available", "How many pins are in the group", id++, true});
  m_model->appendHeaderData(HeaderData{"Ports", "User defined ports", id++, true});
}

std::pair<bool, QString> QLPackagePinsLoader::load(const QString &fileName) {
  initHeader();

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
        bool acceptGroup = false;
        if (!m_model->userGroups().empty()) {
          acceptGroup = m_model->userGroups().contains(group.name);
        } else {
          acceptGroup = true;
        }

        if (acceptGroup) {
          m_model->append(group);
        }
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
