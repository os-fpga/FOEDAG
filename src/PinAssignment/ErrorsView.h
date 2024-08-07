#pragma once

#include <QWidget>

class QTableView;
class QSortFilterProxyModel;

namespace FOEDAG {

class ErrorsModel;

class ErrorsView : public QWidget {
  Q_OBJECT

public:
  ErrorsView(ErrorsModel* model, QWidget* parent = nullptr);

  void setData(const QVector<QVector<QString>>& data);

signals:
  void openFileRequested();

private:
  QTableView* m_tableView{nullptr};
  ErrorsModel* m_sourceModel{nullptr};
  QSortFilterProxyModel* m_proxyModel{nullptr};
};

} // namespace FOEDAG