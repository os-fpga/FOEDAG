#pragma once

#include <QTableView>

class QAbstractTableModel;

namespace FOEDAG {

class ErrorsView : public QTableView {
  Q_OBJECT

public:
  ErrorsView(QAbstractTableModel* model, QWidget* parent = nullptr);
};

} // namespace FOEDAG