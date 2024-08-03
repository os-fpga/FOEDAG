#include "ErrorsModel.h"


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
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  return m_data[index.row()][index.column()];
}

QVariant ErrorsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal) {
    switch(section) {
      case 0: return "Line Num";
      case 1: return "Line";
      case 2: return "Error";
    }
  }
  return QVariant();
}

} // namespace FOEDAG
