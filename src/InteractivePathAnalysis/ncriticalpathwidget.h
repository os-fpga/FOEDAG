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

#ifdef ENABLE_OPEN_FILE_FEATURE
    void openFile(const QString&);
#endif

private:
    class NCriticalPathModel* m_model = nullptr;
    class NCriticalPathView* m_view = nullptr;
    class NCriticalPathToolsWidget* m_toolsWidget = nullptr;
    class NCriticalPathStatusBar* m_statusBar = nullptr;

    Client m_client;
};
