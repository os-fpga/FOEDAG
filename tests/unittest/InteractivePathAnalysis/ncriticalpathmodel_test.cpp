#include "InteractivePathAnalysis/ncriticalpathmodel.h"
#include "InteractivePathAnalysis/ncriticalpathitem.h"

#include <QFile>
#include <QTextStream>
#include <QSignalSpy>
#include <QTimer>
#include <QDebug>

#include "gtest/gtest.h"

namespace  {

QString getModelDataString(const QString& size) {
    static QList<QString> sizes{"full", "short"};
    bool result = sizes.contains(size);
    EXPECT_TRUE(result);

    QFile file(QString(":/InteractivePathAnalysis/data/report_timing.setup.rpt.%1.sample").arg(size));
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

QString toString(const std::map<QString, int>& m) {
    QString result;
    for (const auto& [key, value]: m) {
        result += key + ":" + QString::number(value) + ";";
    }
    return result;
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

} // namespace

TEST(NCriticalPathModel, Paths)
{
    NCriticalPathModel model;

    EXPECT_EQ("", toString(model.inputNodes()));
    EXPECT_EQ("", toString(model.outputNodes()));

    // load data
    QString data{getModelDataString("full")};

    QSignalSpy loadFinishedSpy(&model, &NCriticalPathModel::loadFinished);
    model.loadFromString(data);
    EXPECT_TRUE(waitSignal(loadFinishedSpy));

    int pathsCounter = 0;
    for (int i=0; i<model.rowCount(); ++i) {
        QModelIndex index = model.index(i, 0);
        NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
        if (item && item->isPath()) {
            pathsCounter++;
            QString displayData = model.data(index, Qt::DisplayRole).toString();
            EXPECT_TRUE(item->isSelectable());
            EXPECT_TRUE(displayData.startsWith(QString("#Path %1\n").arg(pathsCounter)));
            EXPECT_TRUE(displayData.contains(QString("\nStartpoint")));
            EXPECT_TRUE(displayData.contains(QString("\nEndpoint")));
        }
    }

    EXPECT_EQ(48, pathsCounter);
    EXPECT_EQ(54, model.rowCount());

    EXPECT_EQ("count[0].Q[0]:2;count[10].Q[0]:1;count[11].Q[0]:1;count[12].Q[0]:1;count[13].Q[0]:1;count[14].Q[0]:1;count[15].Q[0]:1;count[1].Q[0]:2;count[2].Q[0]:15;count[3].Q[0]:1;count[4].Q[0]:1;count[5].Q[0]:1;count[6].Q[0]:1;count[7].Q[0]:1;count[8].Q[0]:1;count[9].Q[0]:1;enable.inpad[0]:16;", toString(model.inputNodes()));
    EXPECT_EQ("count[0].D[0]:1;count[0].E[0]:1;count[10].D[0]:1;count[10].E[0]:1;count[11].D[0]:1;count[11].E[0]:1;count[12].D[0]:1;count[12].E[0]:1;count[13].D[0]:1;count[13].E[0]:1;count[14].D[0]:1;count[14].E[0]:1;count[15].D[0]:1;count[15].E[0]:1;count[1].D[0]:1;count[1].E[0]:1;count[2].D[0]:1;count[2].E[0]:1;count[3].D[0]:1;count[3].E[0]:1;count[4].D[0]:1;count[4].E[0]:1;count[5].D[0]:1;count[5].E[0]:1;count[6].D[0]:1;count[6].E[0]:1;count[7].D[0]:1;count[7].E[0]:1;count[8].D[0]:1;count[8].E[0]:1;count[9].D[0]:1;count[9].E[0]:1;out:16;", toString(model.outputNodes()));

    // clear
    QSignalSpy clearedSpy(&model, &NCriticalPathModel::cleared);
    model.clear();
    EXPECT_TRUE(waitSignal(clearedSpy));

    EXPECT_EQ(0, model.rowCount());

    EXPECT_EQ("", toString(model.inputNodes()));
    EXPECT_EQ("", toString(model.outputNodes()));
}

TEST(NCriticalPathModel, Segments)
{
    NCriticalPathModel model;

    model.loadFromString(getModelDataString("short"));

    QList<NCriticalPathItem*> pathItems;

    for (int i=0; i<model.rowCount(); ++i) {
        QModelIndex index = model.index(i, 0);
        NCriticalPathItem* item = static_cast<NCriticalPathItem*>(index.internalPointer());
        if (item && item->isPath()) {
            pathItems << item;
        }
    }

    EXPECT_EQ(1, pathItems.size());
    EXPECT_EQ(7, model.rowCount());

    NCriticalPathItem* path1 = pathItems.at(0);

    using Row = std::tuple<QString, QString, QString>;
    QList<Row> rows;

    rows << Row{"Path Type : setup",                                                          "",          ""};
    rows << Row{"",                                                                           "",          ""};
    rows << Row{"Point",                                                                      "Incr",      "Path"};
    rows << Row{"------------------------------------------------------------------------------------------", "", ""};
    rows << Row{"clock clk (rise edge)",                                                      "0.000",     "0.000"};
    rows << Row{"clock source latency",                                                       "0.000",     "0.000"};
    rows << Row{"clk.inpad[0] (.input)",                                                      "0.000",     "0.000"};
    rows << Row{"count[2].C[0] (dffsre)",                                                     "0.715",     "0.715"};
    rows << Row{"count[2].Q[0] (dffsre) [clock-to-output]",                                   "0.286",     "1.001"};
    rows << Row{"count_adder_carry_p_cout[3].p[0] (adder_carry)",                             "0.815",     "1.816"};
    rows << Row{"count_adder_carry_p_cout[3].cout[0] (adder_carry)",                          "0.068",     "1.884"};
    rows << Row{"count_adder_carry_p_cout[4].cin[0] (adder_carry)",                           "0.053",     "1.937"};
    rows << Row{"count_adder_carry_p_cout[4].cout[0] (adder_carry)",                          "0.070",     "2.007"};
    rows << Row{"count_adder_carry_p_cout[5].cin[0] (adder_carry)",                           "0.043",     "2.050"};
    rows << Row{"count_adder_carry_p_cout[5].cout[0] (adder_carry)",                          "0.070",     "2.119"};
    rows << Row{"count_adder_carry_p_cout[6].cin[0] (adder_carry)",                           "0.053",     "2.172"};
    rows << Row{"count_adder_carry_p_cout[6].cout[0] (adder_carry)",                          "0.070",     "2.242"};
    rows << Row{"count_adder_carry_p_cout[7].cin[0] (adder_carry)",                           "0.043",     "2.285"};
    rows << Row{"count_adder_carry_p_cout[7].cout[0] (adder_carry)",                          "0.070",     "2.355"};
    rows << Row{"count_adder_carry_p_cout[8].cin[0] (adder_carry)",                           "0.053",     "2.408"};
    rows << Row{"count_adder_carry_p_cout[8].cout[0] (adder_carry)",                          "0.070",     "2.477"};
    rows << Row{"count_adder_carry_p_cout[9].cin[0] (adder_carry)",                           "0.043",     "2.521"};
    rows << Row{"count_adder_carry_p_cout[9].cout[0] (adder_carry)",                          "0.070",     "2.590"};
    rows << Row{"count_adder_carry_p_cout[10].cin[0] (adder_carry)",                          "0.053",     "2.643"};
    rows << Row{"count_adder_carry_p_cout[10].cout[0] (adder_carry)",                         "0.070",     "2.713"};
    rows << Row{"count_adder_carry_p_cout[11].cin[0] (adder_carry)",                          "0.043",     "2.756"};
    rows << Row{"count_adder_carry_p_cout[11].cout[0] (adder_carry)",                         "0.070",     "2.826"};
    rows << Row{"count_adder_carry_p_cout[12].cin[0] (adder_carry)",                          "0.053",     "2.879"};
    rows << Row{"count_adder_carry_p_cout[12].cout[0] (adder_carry)",                         "0.070",     "2.948"};
    rows << Row{"count_adder_carry_p_cout[13].cin[0] (adder_carry)",                          "0.043",     "2.992"};
    rows << Row{"count_adder_carry_p_cout[13].cout[0] (adder_carry)",                         "0.070",     "3.061"};
    rows << Row{"count_adder_carry_p_cout[14].cin[0] (adder_carry)",                          "0.053",     "3.114"};
    rows << Row{"count_adder_carry_p_cout[14].sumout[0] (adder_carry)",                       "0.040",     "3.155"};
    rows << Row{"count_dffsre_Q_D[13].in[0] (.names)",                                        "0.764",     "3.919"};
    rows << Row{"count_dffsre_Q_D[13].out[0] (.names)",                                       "0.228",     "4.147"};
    rows << Row{"count[13].D[0] (dffsre)",                                                    "0.000",     "4.147"};
    rows << Row{"data arrival time",                                                          "",          "4.147"};
    rows << Row{"",                                                                           "",          ""};
    rows << Row{"clock clk (rise edge)",                                                      "0.000",     "0.000"};
    rows << Row{"clock source latency",                                                       "0.000",     "0.000"};
    rows << Row{"clk.inpad[0] (.input)",                                                      "0.000",     "0.000"};
    rows << Row{"count[13].C[0] (dffsre)",                                                    "0.715",     "0.715"};
    rows << Row{"clock uncertainty",                                                          "0.000",     "0.715"};
    rows << Row{"cell setup time",                                                            "-0.057",    "0.659"};
    rows << Row{"data required time",                                                         "",          "0.659"};
    rows << Row{"------------------------------------------------------------------------------------------", "", ""};
    rows << Row{"data required time",                                                         "",          "0.659"};
    rows << Row{"data arrival time",                                                          "",          "-4.147"};
    rows << Row{"------------------------------------------------------------------------------------------", "", ""};
    rows << Row{"slack (VIOLATED)",                                                           "",          "-3.488"};
    rows << Row{"",                                                                           "",          ""};
    rows << Row{"",                                                                           "",          ""};

    for (int i=0; i<rows.size(); ++i) {
        const Row& row = rows[i];
        NCriticalPathItem* segment = path1->child(i);
        const auto& [col0Val, col1Val, col2Val] = row;
        EXPECT_EQ(col0Val, segment->data(0).toString());
        EXPECT_EQ(col1Val, segment->data(1).toString());
        EXPECT_EQ(col2Val, segment->data(2).toString());
    }
}

