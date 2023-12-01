#pragma once

#include <QWidget>
#include <QProcess>

#ifndef STANDALONE_APP
#include "../Compiler/Compiler.h"
#endif

#include "ncriticalpathparameters.h"
#include "client/process.h"

class CustomMenu;

class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class RefreshIndicatorButton;

class NCriticalPathToolsWidget : public QWidget
{
    Q_OBJECT
public:
    NCriticalPathToolsWidget(
        #ifndef STANDALONE_APP
            FOEDAG::Compiler*,
        #endif
        QWidget* parent = nullptr
        );

    ~NCriticalPathToolsWidget()=default;

    void resetConfigurationUI();

    NCriticalPathParametersPtr parameters() const { return m_parameters; }
    void refreshCritPathContextOnSettingsChanged();

public slots:
    void onConnectionStatusChanged(bool);
    void onPathListReceived();
    void onHightLightModeReceived();
    void tryRunPnRView();
    void deactivatePlaceAndRouteViewProcess();

signals:
    void pathListRequested(const QString&);
    void PnRViewRunStatusChanged(bool);
    void highLightModeChanged();
    void isFlatRoutingOnDetected();

private:
#ifdef STANDALONE_APP
    QLineEdit* m_leProjectLocation = nullptr;
    QLineEdit* m_leVprFilePath = nullptr;
    QLineEdit* m_leHardwareXmlFilePath = nullptr;
#else
    FOEDAG::Compiler* m_compiler = nullptr;
#endif
    QLineEdit* m_leNCriticalPathNum = nullptr;
    QComboBox* m_cbHighlightMode = nullptr;
    QComboBox* m_cbPathType = nullptr;
    QComboBox* m_cbDetail = nullptr;
    QCheckBox* m_cbIsFlatRouting = nullptr;

    Process m_vprProcess;

    NCriticalPathParametersPtr m_parameters;

    CustomMenu* m_pathsOptionsMenu = nullptr;
#ifdef STANDALONE_APP
    CustomMenu* m_FOEDAGProjMenu = nullptr;
#endif

    QPushButton* m_bnRunPnRView = nullptr;

    void setupCriticalPathsOptionsMenu(QPushButton*);
#ifdef STANDALONE_APP
    void setupProjectMenu(QPushButton*);
#endif

    QString projectLocation();
    QString vprBaseCommand();
};

