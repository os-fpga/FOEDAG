#pragma once

#include <QWidget>

#include "client/GateIO.h"

#include "../Compiler/Compiler.h"

namespace FOEDAG {

class NCriticalPathWidget: public QWidget
{
    Q_OBJECT

public:
    explicit NCriticalPathWidget(FOEDAG::Compiler*, const QString& logFilePath, const std::filesystem::path& settingsFilePath = "", QWidget* parent = nullptr);
    ~NCriticalPathWidget();

private slots:
    void onFlatRoutingOnDetected();
    void onFlatRoutingOffDetected();
    void requestPathList(const QString& initiator);

private:
    bool m_prevIsFlatRoutingFlag = false;
    
    class NCriticalPathView* m_view = nullptr;
    class NCriticalPathToolsWidget* m_toolsWidget = nullptr;
    class NCriticalPathStatusBar* m_statusBar = nullptr;

    client::GateIO m_gateIO;

    void notifyError(QString, QString);
};

} // namespace FOEDAG