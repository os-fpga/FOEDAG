#pragma once

#include <QTableView>

class QAbstractTableModel;
class QSortFilterProxyModel;

namespace FOEDAG {

class ErrorsView : public QTableView {
  Q_OBJECT

public:
  ErrorsView(QAbstractTableModel* model, QWidget* parent = nullptr);

private:
  QSortFilterProxyModel* m_proxyModel;
};

} // namespace FOEDAG