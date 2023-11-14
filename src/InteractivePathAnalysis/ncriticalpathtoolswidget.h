#pragma once

#include <QWidget>
#include <QProcess>
#include <QTimer>

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

    NCriticalPathParametersPtr parameters() const { return m_parameters; }

public slots:
    void onConnectionStatusChanged(bool);
    void onPathListReceived();

signals:
    void pathListRequested(const QString&);
    void PnRViewRunStatusChanged(bool);
    void highLightModeChanged();

private:
    bool m_isFirstTimeConnectedToParticularPnRViewInstance = true;
#ifdef STANDALONE_APP
    QLineEdit* m_leProj = nullptr;
    QCheckBox* m_cbIsFlatRouting = nullptr;
#else
    FOEDAG::Compiler* m_compiler = nullptr;
#endif
    QLineEdit* m_leNCriticalPathNum = nullptr;
    QComboBox* m_cbHighlightMode = nullptr;
    QComboBox* m_cbPathType = nullptr;
    QComboBox* m_cbDetail = nullptr;
    QCheckBox* m_bnAutoRefreshPathList = nullptr;
    Process m_process;

    NCriticalPathParametersPtr m_parameters;

    CustomMenu* m_pathsOptionsMenu = nullptr;
#ifdef STANDALONE_APP
    CustomMenu* m_FOEDAGProjMenu = nullptr;
#endif

    RefreshIndicatorButton* m_bnRequestPathList = nullptr;
    QPushButton* m_bnRunPnRView = nullptr;

    void setupCriticalPathsOptionsMenu(QPushButton*);
#ifdef STANDALONE_APP
    void setupProjectMenu(QPushButton*);
#endif

    QString projectLocation();
    QString vprBaseCommand();

    void runPnRView();
    void saveCriticalPathsSettings();
};

