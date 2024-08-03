#pragma once

#include <QTableView>

namespace FOEDAG {

class ErrorsView : public QTableView {
  Q_OBJECT

public:
  ErrorsView(QWidget* parent = nullptr);
};

} // namespace FOEDAG