#pragma once


#include <QString>

#include <memory>

class NCriticalPathParameters {
public:
    NCriticalPathParameters();
    ~NCriticalPathParameters()=default;

    void saveToSettings();

    void setCriticalPathNum(int criticalPathNum) { m_criticalPathNum = criticalPathNum; }
    void setPathType(const QString& pathType) { m_pathType = pathType; }
    void setDetailLevel(int detailLevel) { m_detailLevel = detailLevel; }
#ifdef STANDALONE_APP
    void setFlatRouting(bool isFlatRouting) { m_isFlatRouting = isFlatRouting; }
#endif
    void setHighLightMode(int highLightMode) { m_highLightMode = highLightMode; }

    int getCriticalPathNum() const { return m_criticalPathNum; }
    const QString& getPathType() { return m_pathType; }
    int getDetailLevel() { return m_detailLevel; }
    bool getIsFlatRouting() const;
    int getHighLightMode() { return m_highLightMode; }

private:
    int m_criticalPathNum;
    QString m_pathType;
    int m_detailLevel;
#ifdef STANDALONE_APP
    bool m_isFlatRouting;
#endif
    int m_highLightMode;

    void load();
};

using NCriticalPathParametersPtr = std::shared_ptr<NCriticalPathParameters>;
