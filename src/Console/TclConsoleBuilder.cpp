#include "TclConsoleBuilder.h"

namespace FOEDAG {

QWidget *createConsole(TclInterp *interp,
                       std::unique_ptr<ConsoleInterface> iConsole,
                       StreamBuffer *buffer, QWidget *parent,
                       TclConsoleWidget **consolePtr) {
  QWidget *w = new QWidget{parent};
  w->setLayout(new QGridLayout);
  TclConsoleWidget *console =
      new TclConsoleWidget{interp, std::move(iConsole), buffer, w};

  SearchWidget *search = new SearchWidget{console};
  QObject::connect(console, &TclConsoleWidget::searchEnable, search,
                   &SearchWidget::search);

  w->layout()->addWidget(console);
  w->layout()->addWidget(search);
  w->layout()->setSpacing(0);
  w->layout()->setContentsMargins(0, 0, 0, 0);
  w->setGeometry(0, 0, 730, 440);

  if (consolePtr) *consolePtr = console;

  return w;
}

}  // namespace FOEDAG
