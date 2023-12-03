#pragma once

#include <QWidget>

#include "client/client.h"

#include "../Compiler/Compiler.h"

class NCriticalPathWidget: public QWidget
{
    Q_OBJECT

public:
    explicit NCriticalPathWidget(FOEDAG::Compiler*, QWidget* parent = nullptr);
    ~NCriticalPathWidget();

private slots:
    void onFlatRoutingOnDetected();
    void requestPathList(const QString& initiator);

private:
    bool m_prevIsFlatRoutingFlag = false;
    
    class NCriticalPathModel* m_model = nullptr;
    class NCriticalPathFilterModel* m_filterModel = nullptr;
    class NCriticalPathView* m_view = nullptr;
    class NCriticalPathToolsWidget* m_toolsWidget = nullptr;
    class NCriticalPathStatusBar* m_statusBar = nullptr;

    Client m_client;

    void notifyError(QString, QString);
};
