#include "TclConsoleBuilder.h"

#include <QToolButton>

namespace FOEDAG {

QWidget *createConsole(TclInterp *interp,
                       std::unique_ptr<ConsoleInterface> iConsole,
                       TclConsoleBuffer *buffer, QWidget *parent,
                       TclConsoleWidget **consolePtr) {
  QWidget *w = new QWidget{parent};
  auto layout = new QGridLayout;
  w->setLayout(layout);
  TclConsoleWidget *console =
      new TclConsoleWidget{interp, std::move(iConsole), buffer, w};

  SearchWidget *search = new SearchWidget{console};
  QObject::connect(console, &TclConsoleWidget::searchEnable, search,
                   &SearchWidget::search);

  QVBoxLayout *buttonsLayout = new QVBoxLayout;

  auto tool = new QToolButton{};
  tool->setToolTip(QString{"Clear console"});
  QObject::connect(tool, &QToolButton::clicked, console,
                   &TclConsoleWidget::clearText);
  tool->setIcon(QIcon{":/images/erase.png"});
  buttonsLayout->addWidget(tool);

  tool = new QToolButton{};
  tool->setToolTip(QString{"Find..."});
  QObject::connect(tool, &QToolButton::clicked, console,
                   &TclConsoleWidget::searchEnable);
  tool->setIcon(QIcon{":/images/search.png"});
  buttonsLayout->addWidget(tool);
  layout->addLayout(buttonsLayout, 0, 0, Qt::AlignTop);

  layout->addWidget(console, 0, 1);
  layout->addWidget(search, 1, 1);
  layout->setSpacing(1);
  layout->setContentsMargins(0, 0, 0, 0);
  w->setGeometry(0, 0, 730, 440);

  if (consolePtr) *consolePtr = console;

  return w;
}

}  // namespace FOEDAG
