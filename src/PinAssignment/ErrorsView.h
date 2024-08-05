#pragma once

#include <QTableView>

class QSortFilterProxyModel;

namespace FOEDAG {

class ErrorsModel;

class ErrorsView : public QTableView {
  Q_OBJECT

public:
  ErrorsView(ErrorsModel* model, QWidget* parent = nullptr);

  void setData(const QVector<QVector<QString>>& data);

private:
  ErrorsModel* m_sourceModel{nullptr};
  QSortFilterProxyModel* m_proxyModel{nullptr};
};

} // namespace FOEDAG