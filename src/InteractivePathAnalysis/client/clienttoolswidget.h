#pragma once

#include <QWidget>
#include <QProcess>
#include <QTimer>

#ifndef STANDALONE_APP
#include "../../Compiler/Compiler.h"
#endif

#include "process.h"

class CustomMenu;

class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class PushButton;

class ClientToolsWidget : public QWidget
{
    Q_OBJECT
public:
    ClientToolsWidget(
        #ifndef STANDALONE_APP
            FOEDAG::Compiler*,
        #endif
        QWidget* parent = nullptr
        );

    ~ClientToolsWidget()=default;

    int nCriticalPathNum() const;
    int isFlatRouting() const;
    QString pathType() const;
    int detailesLevel() const;
    int highlightMode() const;

public slots:
    void onConnectionStatusChanged(bool);
    void onGotPathList();

signals:
    void connectionStatusChanged(bool);
    void getPathListRequested();
    void highLightModeChanged();
    void PnRViewProcessRunningStatus(bool);

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
    QComboBox* m_cbDetailes = nullptr;
    QCheckBox* m_bnAutoRefreshPathList = nullptr;
    Process m_process;

    CustomMenu* m_pathsOptionsMenu = nullptr;
#ifdef STANDALONE_APP
    CustomMenu* m_FOEDAGProjMenu = nullptr;
#endif

    PushButton* m_bnRequestPathList = nullptr;
    QPushButton* m_bnRunPnRView = nullptr;

    void setupPathsOptionsMenu(QPushButton*);
#ifdef STANDALONE_APP
    void setupProjectMenu(QPushButton*);
#endif

    QString projectLocation();
    QString vprBaseCommand();

    void runPnRView();
    void savePathsOptionsSettings();
};

