#pragma once

#include <QWidget>
#include <QProcess>

#include "../Compiler/Compiler.h"

#include "NCriticalPathParameters.h"
#include "Process.h"

class CustomMenu;

class QLabel;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class RefreshIndicatorButton;

#define USE_DRAW_CRITICAL_PATH_CONTOUR

class NCriticalPathToolsWidget : public QWidget
{
    Q_OBJECT
public:
    NCriticalPathToolsWidget(FOEDAG::Compiler*, QWidget* parent = nullptr);
    ~NCriticalPathToolsWidget()=default;

    void resetConfigurationUI();

    NCriticalPathParametersPtr parameters() const { return m_parameters; }
    void refreshCritPathContextOnSettingsChanged();

public slots:
    void onConnectionStatusChanged(bool);
    void tryRunPnRView();
    void deactivatePlaceAndRouteViewProcess();

signals:
    void pathListRequested(const QString&);
    void PnRViewRunStatusChanged(bool);
    void highLightModeChanged();
    void isFlatRoutingOnDetected();
    void vprProcessErrorOccured(QString);
    void serverPortNumDetected(int);

private:
    FOEDAG::Compiler* m_compiler = nullptr;

    QLineEdit* m_leNCriticalPathNum = nullptr;
    QComboBox* m_cbHighlightMode = nullptr;
#ifdef USE_DRAW_CRITICAL_PATH_CONTOUR
    QCheckBox* m_cbDrawPathContour = nullptr;
#endif
    QComboBox* m_cbPathType = nullptr;
    QComboBox* m_cbDetail = nullptr;
    QCheckBox* m_cbIsFlatRouting = nullptr;
    QCheckBox* m_cbIsLogToFileEnabled = nullptr;

    Process m_vprProcess;

    NCriticalPathParametersPtr m_parameters;

    CustomMenu* m_pathsOptionsMenu = nullptr;

    QPushButton* m_bnRunPnRView = nullptr;

    void setupCriticalPathsOptionsMenu(QPushButton*);

    QString projectLocation();
    QString vprBaseCommand();
};

