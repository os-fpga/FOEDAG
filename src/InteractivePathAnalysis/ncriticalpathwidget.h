#pragma once

#include <QWidget>

#include "client/client.h"

#ifndef STANDALONE_APP
#include "../Compiler/Compiler.h"
#endif

class NCriticalPathWidget: public QWidget
{
    Q_OBJECT

public:
    explicit NCriticalPathWidget(
#ifndef STANDALONE_APP
        FOEDAG::Compiler*,
#endif
        QWidget* parent = nullptr);
    ~NCriticalPathWidget();

    static const QString& name() { return s_name; }

#ifdef ENABLE_OPEN_FILE_FEATURE
    void openFile(const QString&);
#endif

private slots:
    void onFlatRoutingOnDetected();
    void requestPathList(const QString& initiator);

private:
    static QString s_name;
    bool m_prevIsFlatRoutingFlag = false;
    
    class NCriticalPathModel* m_model = nullptr;
    class NCriticalPathFilterModel* m_filterModel = nullptr;
    class NCriticalPathView* m_view = nullptr;
    class NCriticalPathToolsWidget* m_toolsWidget = nullptr;
    class NCriticalPathStatusBar* m_statusBar = nullptr;

    Client m_client;
};
