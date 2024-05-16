/**
  * @file NCriticalPathModel_test.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
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

#include "InteractivePathAnalysis/NCriticalPathModel.h"
#include "InteractivePathAnalysis/NCriticalPathItem.h"
#include "InteractivePathAnalysis/NCriticalPathFilterModel.h"

#include <QFile>
#include <QTextStream>
#include <QSignalSpy>
#include <QTimer>
#include <QDebug>

#include "gtest/gtest.h"

using namespace FOEDAG;
namespace  {

const int EXPECTED_CRIT_PATH_NUM = 48;
#define EXPECT_QSTREQ(s1, s2) EXPECT_STREQ((s1).toStdString().c_str(), (s2).toStdString().c_str())

QString getRawModelDataString() {
    QFile file(QString(":/InteractivePathAnalysis/data/report_timing.setup.rpt.sample"));
    QString fileContent;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        fileContent = in.readAll();
        file.close();
    } else {
        qDebug() << "Failed to open the file:" << file.errorString();
    }

    return fileContent;
}

const std::map<int, QString>& getOtherExpected() {
    static std::map<int, QString> otherExpectated = {
        {1, "#Timing report of worst 48 path(s)"},
        {2, "# Unit scale: 1e-09 seconds"},
        {3, "# Output precision: 3"},
        {4, ""},
        {5, "#End of timing report"}
    };
    return otherExpectated;
}

const std::map<int, QString>& getPathExpected() {
    static std::map<int, QString> pathExpectated = {
        {1, "#Path 1\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[13].D[0] (dffsre clocked by clk)"},
        {2, "#Path 2\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[12].D[0] (dffsre clocked by clk)"},
        {3, "#Path 3\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[14].D[0] (dffsre clocked by clk)"},
        {4, "#Path 4\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[15].D[0] (dffsre clocked by clk)"},
        {5, "#Path 5\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[10].D[0] (dffsre clocked by clk)"},
        {6, "#Path 6\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[11].D[0] (dffsre clocked by clk)"},
        {7, "#Path 7\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[8].D[0] (dffsre clocked by clk)"},
        {8, "#Path 8\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[9].D[0] (dffsre clocked by clk)"},
        {9, "#Path 9\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[6].D[0] (dffsre clocked by clk)"},
        {10, "#Path 10\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[4].D[0] (dffsre clocked by clk)"},
        {10, "#Path 10\nStartpoint: count[2].Q[0] (dffsre clocked by clk)\nEndpoint : count[4].D[0] (dffsre clocked by clk)"},
        {47, "#Path 47\nStartpoint: enable.inpad[0] (.input clocked by clk)\nEndpoint : count[15].E[0] (dffsre clocked by clk)"},
        {48, "#Path 48\nStartpoint: enable.inpad[0] (.input clocked by clk)\nEndpoint : count[13].E[0] (dffsre clocked by clk)"}
    };
    return pathExpectated;
}

std::map<QString, int> getInputNodes() {
    static std::map<QString, int> inputNodes = {
        {"count[0].Q[0]", 2},
        {"count[10].Q[0]", 1},
        {"count[11].Q[0]", 1},
        {"count[12].Q[0]", 1},
        {"count[13].Q[0]", 1},
        {"count[14].Q[0]", 1},
        {"count[15].Q[0]", 1},
        {"count[1].Q[0]", 2},
        {"count[2].Q[0]", 15},
        {"count[3].Q[0]", 1},
        {"count[4].Q[0]", 1},
        {"count[5].Q[0]", 1},
        {"count[6].Q[0]", 1},
        {"count[7].Q[0]", 1},
        {"count[8].Q[0]", 1},
        {"count[9].Q[0]", 1},
        {"enable.inpad[0]", 16}
    };
    return inputNodes;
}

std::map<QString, int> getOutputNodes() {
    static std::map<QString, int> outputNodes = {
        {"count[0].D[0]", 1},
        {"count[0].E[0]", 1},
        {"count[10].D[0]", 1},
        {"count[10].E[0]", 1},
        {"count[11].D[0]", 1},
        {"count[11].E[0]", 1},
        {"count[12].D[0]", 1},
        {"count[12].E[0]", 1},
        {"count[13].D[0]", 1},
        {"count[13].E[0]", 1},
        {"count[14].D[0]", 1},
        {"count[14].E[0]", 1},
        {"count[15].D[0]", 1},
        {"count[15].E[0]", 1},
        {"count[1].D[0]", 1},
        {"count[1].E[0]", 1},
        {"count[2].D[0]", 1},
        {"count[2].E[0]", 1},
        {"count[3].D[0]", 1},
        {"count[3].E[0]", 1},
        {"count[4].D[0]", 1},
        {"count[4].E[0]", 1},
        {"count[5].D[0]", 1},
        {"count[5].E[0]", 1},
        {"count[6].D[0]", 1},
        {"count[6].E[0]", 1},
        {"count[7].D[0]", 1},
        {"count[7].E[0]", 1},
        {"count[8].D[0]", 1},
        {"count[8].E[0]", 1},
        {"count[9].D[0]", 1},
        {"count[9].E[0]", 1},
        {"out", 16}
    };
    return outputNodes;
}

QString getDiffStr(const std::map<QString, int> expected, const std::map<QString, int> actual) 
{
    QList<QString> diffs;
    for (const auto& [key, val]: expected) {
        auto it = actual.find(key);
        if (it == actual.end()) {
            diffs.append("absent_pair={" + key + ":" + QString::number(val) + "}");
        } else {
            if (it->second != val) {
                diffs.append(QString("wrong_value for %1, expected val=%2, actual val=%3").arg(key).arg(QString::number(val)).arg(QString::number(it->second)));
            }
        }
    }
    for (const auto& [key, val]: actual) {
        if (expected.find(key) == expected.end()) {
            diffs.append("contains{" + key + ":" + QString::number(val)+"}");
        }
    }
    return diffs.join(", ");
}

// helper function to catchup signal or leave by timeout
bool waitSignal(QSignalSpy& spy) {
    bool result = false;
    QEventLoop loop;

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true); 
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&loop]() {
        loop.quit();
    });
    timeoutTimer.start(200);

    QTimer signalCheckerTimer;
    QObject::connect(&signalCheckerTimer, &QTimer::timeout, [&loop, &spy, &result]() {
        if (spy.count()) {
            result = true;
            loop.quit();
        }        
    });
    signalCheckerTimer.start(2);

    loop.exec();
    return result;
}

std::pair<QList<NCriticalPathItem*>, QList<NCriticalPathItem*>> collectVisibleItems(NCriticalPathFilterModel& filter)
{
    QList<NCriticalPathItem*> pathItems;
    QList<NCriticalPathItem*> otherItems;
    for (int i=0; i<filter.rowCount(); ++i) {
        QModelIndex index = filter.index(i, 0);
        QModelIndex srcIndex = filter.mapToSource(index);
        NCriticalPathItem* item = static_cast<NCriticalPathItem*>(srcIndex.internalPointer());
        if (item) {
            if (item->isPath()) {
                pathItems << item;
            } else {
                otherItems << item;
            }
        }
    }
    return std::pair<QList<NCriticalPathItem*>, QList<NCriticalPathItem*>>{pathItems, otherItems};
}

} // namespace

TEST(NCriticalPathModel, Paths)
{
    NCriticalPathModel model;

    // PRE_TEST CHECK
    EXPECT_QSTREQ(QString{""}, getDiffStr(std::map<QString, int>{}, model.inputNodes()));
    EXPECT_QSTREQ(QString{""}, getDiffStr(std::map<QString, int>{}, model.outputNodes()));

    // LOAD DATA
    QSignalSpy loadFinishedSpy(&model, &NCriticalPathModel::loadFinished);
    model.loadFromString(getRawModelDataString());
    EXPECT_TRUE(waitSignal(loadFinishedSpy));

    // TEST DATA
    int otherCounter = 0;
    int pathsCounter = 0;
    for (int i=0; i<model.rowCount(); ++i) {
        QModelIndex index = model.index(i, 0);
        NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
        if (item) {
            QString displayData = model.data(index, Qt::DisplayRole).toString();
            if (item->isPath()) {
                pathsCounter++;
                if (getPathExpected().find(pathsCounter) != getPathExpected().end()) {
                    EXPECT_QSTREQ(getPathExpected().at(pathsCounter), displayData);
                }
                EXPECT_TRUE(item->isSelectable());
                EXPECT_TRUE(displayData.startsWith(QString("#Path %1\n").arg(pathsCounter)));
                EXPECT_TRUE(displayData.contains(QString("\nStartpoint")));
                EXPECT_TRUE(displayData.contains(QString("\nEndpoint")));
            } else {
                otherCounter++;
                EXPECT_QSTREQ(getOtherExpected().at(otherCounter), displayData);
            }
        }
    }

    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathsCounter);
    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), model.rowCount());

    EXPECT_QSTREQ(QString{""}, getDiffStr(getInputNodes(), model.inputNodes()));
    EXPECT_QSTREQ(QString{""}, getDiffStr(getOutputNodes(), model.outputNodes()));

    // CLEAR DATA
    QSignalSpy clearedSpy(&model, &NCriticalPathModel::cleared);
    model.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, model.rowCount());

    EXPECT_QSTREQ(QString{""}, getDiffStr(std::map<QString, int>{}, model.inputNodes()));
    EXPECT_QSTREQ(QString{""}, getDiffStr(std::map<QString, int>{}, model.outputNodes()));
}


// TODO: Previous test case wasn't compatible with current implementation. Moreover, the implementation will be changed soon (see https://github.com/QL-Proprietary/aurora2/issues/481)
// Test case will be revised after/along with a new method of extracting path elements implementation.
// TEST(NCriticalPathModel, Path1Segments)
// {
// }

TEST(NCriticalPathFilterModel, NoFilterCriteria)
{
    NCriticalPathModel source;
    NCriticalPathFilterModel filter;
    filter.setSourceModel(&source);

    // LOAD DATA
    QSignalSpy loadFinishedSpy(&source, &NCriticalPathModel::loadFinished);
    source.loadFromString(getRawModelDataString());
    EXPECT_TRUE(waitSignal(loadFinishedSpy));

    // TEST DATA
    auto [pathItems, otherItems] = collectVisibleItems(filter);

    EXPECT_QSTREQ(QString{""}, getDiffStr(getInputNodes(), source.inputNodes()));
    EXPECT_QSTREQ(QString{""}, getDiffStr(getOutputNodes(), source.outputNodes()));

    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems.size());
    EXPECT_EQ(getOtherExpected().size(), otherItems.size());
    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());

    // CLEAR DATA
    QSignalSpy clearedSpy(&source, &NCriticalPathModel::cleared);
    source.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, source.rowCount());
    EXPECT_EQ(0, filter.rowCount());
}

TEST(NCriticalPathFilterModel, InputFilterCriteriaExactMatchCaseInsensitive)
{
    NCriticalPathModel source;
    NCriticalPathFilterModel filter;
    filter.setSourceModel(&source);

    // PRE-TEST
    {
        QSignalSpy loadFinishedSpy(&source, &NCriticalPathModel::loadFinished);
        source.loadFromString(getRawModelDataString());
        EXPECT_TRUE(waitSignal(loadFinishedSpy));

        auto [pathItems, otherItems] = collectVisibleItems(filter);

        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems.size());
        EXPECT_EQ(getOtherExpected().size(), otherItems.size());
        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());
    }

    /// APPLY FILTER
    {
        FilterCriteriaConf inputConf{"CoUnT[1]", false, false};
        FilterCriteriaConf outputConf;

        filter.setFilterCriteria(inputConf, outputConf);

        int pathsCounter = 0;
        int otherCounter = 0;
        for (int i=0; i<filter.rowCount(); ++i) {
            QModelIndex index = filter.index(i, 0);
            QModelIndex srcIndex = filter.mapToSource(index);
            NCriticalPathItem* item = static_cast<NCriticalPathItem*>(srcIndex.internalPointer());
            if (item) {
                QString displayData = source.data(srcIndex, Qt::DisplayRole).toString();
                if (item->isPath()) {
                    pathsCounter++;
                    QRegularExpression regex("Startpoint:\\s+count\\[1\\]");
                    EXPECT_TRUE(regex.match(displayData).hasMatch());
                } else {
                    otherCounter++;
                    EXPECT_QSTREQ(getOtherExpected().at(otherCounter), displayData);
                }
            }
        }

        EXPECT_EQ(2, pathsCounter);
        EXPECT_EQ(getOtherExpected().size(), otherCounter);
        EXPECT_EQ(pathsCounter + getOtherExpected().size(), filter.rowCount());
    }

    /// RESET FILTER
    {
        filter.clear();

        auto [pathItems2, otherItems2] = collectVisibleItems(filter);

        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems2.size());
        EXPECT_EQ(getOtherExpected().size(), otherItems2.size());
        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());
    }

    // CLEAR DATA
    QSignalSpy clearedSpy(&source, &NCriticalPathModel::cleared);
    source.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, source.rowCount());
    EXPECT_EQ(0, filter.rowCount());
}

TEST(NCriticalPathFilterModel, InputFilterCriteriaExactMatchCaseSensitive)
{
    NCriticalPathModel source;
    NCriticalPathFilterModel filter;
    filter.setSourceModel(&source);

    // PRE-TEST
    {
        QSignalSpy loadFinishedSpy(&source, &NCriticalPathModel::loadFinished);
        source.loadFromString(getRawModelDataString());
        EXPECT_TRUE(waitSignal(loadFinishedSpy));

        auto [pathItems, otherItems] = collectVisibleItems(filter);

        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems.size());
        EXPECT_EQ(getOtherExpected().size(), otherItems.size());
        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());
    }

    /// APPLY FILTER 1
    {
        FilterCriteriaConf inputConf{"CoUnT[1]", true, false};
        FilterCriteriaConf outputConf;

        filter.setFilterCriteria(inputConf, outputConf);

        auto [pathItems, otherItems] = collectVisibleItems(filter);

        EXPECT_EQ(0, pathItems.size());
        EXPECT_EQ(getOtherExpected().size(), otherItems.size());
        EXPECT_EQ(pathItems.size() + getOtherExpected().size(), filter.rowCount());
    }

    /// APPLY FILTER 2
    {
        FilterCriteriaConf inputConf{"count[1]", true, false};
        FilterCriteriaConf outputConf;

        filter.setFilterCriteria(inputConf, outputConf);

        int pathsCounter = 0;
        int otherCounter = 0;
        for (int i=0; i<filter.rowCount(); ++i) {
            QModelIndex index = filter.index(i, 0);
            QModelIndex srcIndex = filter.mapToSource(index);
            NCriticalPathItem* item = static_cast<NCriticalPathItem*>(srcIndex.internalPointer());
            if (item) {
                QString displayData = source.data(srcIndex, Qt::DisplayRole).toString();
                if (item->isPath()) {
                    pathsCounter++;
                    QList<QString> segments = displayData.split("\n");
                    for (const QString& segment: segments) {
                        if (segment.startsWith("Startpoint")) {
                            QRegularExpression regex("count\\[1\\]");
                            EXPECT_TRUE(regex.match(displayData).hasMatch());
                        }
                    }
                } else {
                    otherCounter++;
                    EXPECT_QSTREQ(getOtherExpected().at(otherCounter), displayData);
                }
            }
        }

        EXPECT_EQ(2, pathsCounter);
        EXPECT_EQ(getOtherExpected().size(), otherCounter);
        EXPECT_EQ(pathsCounter + getOtherExpected().size(), filter.rowCount());
    }

    /// RESET FILTER
    {
        filter.clear();

        auto [pathItems, otherItems] = collectVisibleItems(filter);

        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems.size());
        EXPECT_EQ(getOtherExpected().size(), otherItems.size());
        EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());
    }

    // CLEAR DATA
    QSignalSpy clearedSpy(&source, &NCriticalPathModel::cleared);
    source.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, source.rowCount());
    EXPECT_EQ(0, filter.rowCount());
}

TEST(NCriticalPathFilterModel, OutputFilterCriteriaRegexp)
{
    NCriticalPathModel source;
    NCriticalPathFilterModel filter;
    filter.setSourceModel(&source);


    // LOAD DATA
    QSignalSpy loadFinishedSpy(&source, &NCriticalPathModel::loadFinished);
    source.loadFromString(getRawModelDataString());
    EXPECT_TRUE(waitSignal(loadFinishedSpy));

    // PRE_TEST DATA
    auto [pathItems, otherItems] = collectVisibleItems(filter);

    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM, pathItems.size());
    EXPECT_EQ(getOtherExpected().size(), otherItems.size());
    EXPECT_EQ(EXPECTED_CRIT_PATH_NUM + getOtherExpected().size(), filter.rowCount());

    // APPLY FILTER
    FilterCriteriaConf inputConf;
    FilterCriteriaConf outputConf{"count\\[\\d{2}\\]", false, true};

    filter.setFilterCriteria(inputConf, outputConf);

    int pathsCounter = 0;
    int otherCounter = 0;
    for (int i=0; i<filter.rowCount(); ++i) {
        QModelIndex index = filter.index(i, 0);
        QModelIndex srcIndex = filter.mapToSource(index);
        NCriticalPathItem* item = static_cast<NCriticalPathItem*>(srcIndex.internalPointer());
        if (item) {
            QString displayData = source.data(srcIndex, Qt::DisplayRole).toString();
            if (item->isPath()) {
                pathsCounter++;
                QList<QString> segments = displayData.split("\n");
                for (const QString& segment: segments) {
                    if (segment.startsWith("Endpoint")) {
                        QRegularExpression regex("count\\[\\d{2}\\]");
                        EXPECT_TRUE(regex.match(displayData).hasMatch());
                    }
                }
            } else {
                otherCounter++;
                EXPECT_QSTREQ(getOtherExpected().at(otherCounter), displayData);
            }
        }
    }

    EXPECT_EQ(18, pathsCounter);
    EXPECT_EQ(getOtherExpected().size(), otherCounter);
    EXPECT_EQ(pathsCounter + getOtherExpected().size(), filter.rowCount());

    // CLEAR DATA
    QSignalSpy clearedSpy(&source, &NCriticalPathModel::cleared);
    source.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, source.rowCount());
    EXPECT_EQ(0, filter.rowCount());
}
