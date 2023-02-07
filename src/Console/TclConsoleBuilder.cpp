#include "TclConsoleBuilder.h"

#include <QToolButton>

namespace FOEDAG {

QWidget *createConsole(TclInterp *interp,
                       std::unique_ptr<ConsoleInterface> iConsole,
                       StreamBuffer *buffer, QWidget *parent,
                       TclConsoleWidget **consolePtr) {
  QWidget *w = new QWidget{parent};
  auto layout = new QGridLayout;
  w->setLayout(layout);
  TclConsoleWidget *console =
      new TclConsoleWidget{interp, std::move(iConsole), buffer, w};

  SearchWidget *search = new SearchWidget{console};
  QObject::connect(console, &TclConsoleWidget::searchEnable, search,
                   &SearchWidget::search);

  auto tool = new QToolButton{};
  tool->setToolTip(QString{"Clear console"});
  QObject::connect(tool, &QToolButton::clicked, console,
                   &TclConsoleWidget::clearText);
  tool->setIcon(QIcon{":/images/erase.png"});
  layout->addWidget(tool, 0, 0, Qt::AlignTop);
  layout->addWidget(console, 0, 1);
  layout->addWidget(search, 1, 1);
  layout->setSpacing(1);
  layout->setContentsMargins(0, 0, 0, 0);
  w->setGeometry(0, 0, 730, 440);

  if (consolePtr) *consolePtr = console;

  return w;
}

}  // namespace FOEDAG
