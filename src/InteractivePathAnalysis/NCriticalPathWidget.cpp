/**
  * @file NCriticalPathWidget.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "NCriticalPathWidget.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "../Compiler/QLSettingsManager.h"
#include "NCriticalPathModuleInfo.h"
#include "NCriticalPathStatusBar.h"
#include "NCriticalPathTheme.h"
#include "NCriticalPathToolsWidget.h"
#include "NCriticalPathView.h"
#include "SimpleLogger.h"

namespace FOEDAG {

NCriticalPathWidget::NCriticalPathWidget(
    FOEDAG::Compiler* compiler, const QString& logFilePath,
    const std::filesystem::path& settingsFilePath, QWidget* parent)
    : QWidget(parent),
      m_view(new NCriticalPathView(this)),
      m_toolsWidget(
          new NCriticalPathToolsWidget(compiler, settingsFilePath, this)),
      m_statusBar(new NCriticalPathStatusBar(this)),
      m_gateIO(m_toolsWidget->parameters()) {
  SimpleLogger::instance().setFilePath(logFilePath);

  m_prevIsFlatRoutingFlag =
      m_toolsWidget->parameters()->getIsFlatRouting();  // int prev value

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);

  /// viewer setup
  m_view->header()->setSectionResizeMode(
      QHeaderView::ResizeMode::ResizeToContents);
  m_view->header()->setHidden(true);
  ///

  /// toolbox setup
  QHBoxLayout* toolBarLayout = new QHBoxLayout;
  int borderSize = NCriticalPathTheme::instance().borderSize();
  toolBarLayout->setContentsMargins(0, borderSize, borderSize, borderSize);

  toolBarLayout->addWidget(m_toolsWidget);
  toolBarLayout->addStretch();
  ///

  ///
  layout->addLayout(toolBarLayout);
  layout->addWidget(m_view);
  layout->addWidget(m_statusBar);

  // view connections
  connect(m_view, &NCriticalPathView::pathElementSelectionChanged, &m_gateIO,
          &client::GateIO::requestPathItemsHighLight);
  connect(m_view, &NCriticalPathView::dataLoaded, this, [this]() {
    QString durationStr{
        getPrettyDurationStrFromMs(m_fetchPathListTimer.elapsed()).c_str()};
    m_statusBar->setMessage(tr("Got path list (took %1)").arg(durationStr));
  });

  // toolswidget connections
  connect(m_toolsWidget, &NCriticalPathToolsWidget::PnRViewRunStatusChanged,
          this, [this](bool isRunning) {
            if (!isRunning) {
              m_view->clear();
              m_statusBar->setMessage(tr("P&R View is not running"));
            } else {
              m_statusBar->setMessage(tr("P&R View is starting..."));
            }
            m_gateIO.setServerIsRunning(isRunning);
          });
  connect(m_toolsWidget, &NCriticalPathToolsWidget::pathListRequested, this,
          &NCriticalPathWidget::requestPathList);
  connect(m_toolsWidget, &NCriticalPathToolsWidget::highLightModeChanged,
          &m_gateIO, &client::GateIO::onHightLightModeChanged);
  connect(m_toolsWidget, &NCriticalPathToolsWidget::vprProcessErrorOccured,
          this, [this](QString errorMsg) { notifyError("VPR", errorMsg); });
  connect(m_toolsWidget, &NCriticalPathToolsWidget::serverPortNumDetected,
          &m_gateIO, &client::GateIO::onServerPortDetected);

  // client connections
  connect(&m_gateIO, &client::GateIO::pathListDataReceived, m_view,
          &NCriticalPathView::loadFromString);
  connect(&m_gateIO, &client::GateIO::connectedChanged, this,
          [this](bool isConnected) {
            m_toolsWidget->onConnectionStatusChanged(isConnected);
            m_statusBar->onConnectionStatusChanged(isConnected);
          });

  connect(m_toolsWidget, &NCriticalPathToolsWidget::isFlatRoutingOnDetected,
          this, &NCriticalPathWidget::onFlatRoutingOnDetected);
  connect(m_toolsWidget, &NCriticalPathToolsWidget::isFlatRoutingOffDetected,
          this, &NCriticalPathWidget::onFlatRoutingOffDetected);
  connect(FOEDAG::QLSettingsManager::getInstance(),
          &FOEDAG::QLSettingsManager::settingsChanged, this, [this]() {
            const auto& parameters = m_toolsWidget->parameters();
            parameters->resetChangedFlags();
            if (bool foundChanges = parameters->loadFromFile()) {
              m_toolsWidget->refreshCritPathContextOnSettingsChanged();
              m_toolsWidget->resetConfigurationUI();
            }
          });

  m_toolsWidget->tryRunPnRView();  // startup run
}

NCriticalPathWidget::~NCriticalPathWidget() {}

void NCriticalPathWidget::onFlatRoutingOnDetected() {
  m_toolsWidget->deactivatePlaceAndRouteViewProcess();
  QString msg =
      tr("Place&Route View is disabled since flat_routing is enabled in VPR!");
  m_statusBar->setMessage(msg);
  notifyError("VPR", msg);
}

void NCriticalPathWidget::onFlatRoutingOffDetected() {
  m_toolsWidget->enablePlaceAndRouteViewButton();
  m_statusBar->setMessage("");
}

void NCriticalPathWidget::requestPathList(const QString& initiator) {
  if (m_gateIO.isConnected()) {
    m_view->clear();
    m_view->showBusyOverlay();
    m_gateIO.requestPathList(initiator);
    m_fetchPathListTimer.restart();
    m_statusBar->setMessage(tr("Getting path list..."));
  } else {
    SimpleLogger::instance().error("cannot requestPathList by", initiator,
                                   "because client is not connected");
  }
}

void NCriticalPathWidget::notifyError(QString title, QString errorMsg) {
  if (isVisible()) {
    QMessageBox::warning(this, title, errorMsg, QMessageBox::Ok);
  } else {
    // if parent widget is not visible, the message box will behave as not a
    // modal, to avoid this let's show message box with delay.
    QTimer::singleShot(500, [=]() {
      QMessageBox::warning(this, title, errorMsg, QMessageBox::Ok);
    });
  }
  if (!errorMsg.contains(": warning:")) {  // hide busy widget only on errors
                                           // but not on warnings
    m_view->hideBusyOverlay();
  }
}

}  // namespace FOEDAG
