#include "ErrorsModel.h"

#include <QIcon>

namespace FOEDAG {

ErrorsModel::ErrorsModel(QObject *parent)
: QAbstractTableModel(parent) 
{
}

void ErrorsModel::setData(const QVector<QVector<QString>>& data)
{
  beginResetModel();
  m_data = data;
  endResetModel();
}

int ErrorsModel::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_data.size();
}

int ErrorsModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_data.isEmpty() ? 0 : m_data[0].size();
}

QVariant ErrorsModel::data(const QModelIndex &index, int role) const 
{
  if (!index.isValid()) {
    return QVariant();
  }
  if (role == Qt::DisplayRole) {
    return m_data[index.row()][index.column()];
  } else if ((role == Qt::DecorationRole) && (index.column() == 0)) {
    return QIcon(":/images/error.png");
  }
  return QVariant();
}

QVariant ErrorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal) {
    switch(section) {
      case 0: return "Error";
      case 1: return "Line Num";
      case 2: return "Line";
    }
  }
  return QVariant();
}

} // namespace FOEDAG
