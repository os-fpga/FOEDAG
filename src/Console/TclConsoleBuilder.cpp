#include "TclConsoleBuilder.h"

namespace FOEDAG {

TclConsoleWidget *TclConsoleGLobal::m_tclConsole = nullptr;
TclConsoleWidget *TclConsoleGLobal::tclConsole() { return m_tclConsole; }

void TclConsoleGLobal::setTclConsole(TclConsoleWidget *tclConsole) {
  m_tclConsole = tclConsole;
}

QWidget *createConsole(Tcl_Interp *interp,
                       std::unique_ptr<ConsoleInterface> iConsole,
                       StreamBuffer *buffer, QWidget *parent) {
  QWidget *w = new QWidget{parent};
  w->setLayout(new QGridLayout);
  TclConsoleWidget *console =
      new TclConsoleWidget{interp, std::move(iConsole), buffer};
  TclConsoleGLobal::setTclConsole(console);

  SearchWidget *search = new SearchWidget{console};
  QObject::connect(console, &TclConsoleWidget::searchEnable, search,
                   &SearchWidget::search);

  w->layout()->addWidget(console);
  w->layout()->addWidget(search);
  w->layout()->setSpacing(0);
  w->layout()->setContentsMargins(0, 0, 0, 0);
  w->setGeometry(0, 0, 730, 440);

  return w;
}

}  // namespace FOEDAG
