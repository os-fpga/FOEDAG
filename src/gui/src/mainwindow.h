#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logicunit.h"
#include "container.h"
#include "clockconfig.h"


#include <QDialog>

class ExplorerScene;

QT_BEGIN_NAMESPACE
class QAction;
class QToolBox;
class QSpinBox;
class QComboBox;
class QFontComboBox;
class QButtonGroup;
class QLineEdit;
class QGraphicsTextItem;
class QFont;
class QToolButton;
class QAbstractButton;
class QGraphicsView;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
   MainWindow();

private slots:
   // void backgroundButtonGroupClicked(QAbstractButton *button);
    void simulationButtonGroupClicked(QAbstractButton* button);
    void buttonGroupClicked(int id);
    void powerButtonGroupClicked(QAbstractButton* button);
    void deleteItem();
    void pointerGroupClicked(int id);
    void bringToFront();
    void sendToBack();
    void itemInserted(LogicUnit *item);
    void textInserted(QGraphicsTextItem *item);
    void currentFontChanged(const QFont &font);
    void fontSizeChanged(const QString &size);
    void sceneScaleChanged(int scale);
    void activityCycleCountChangedChanged(int number);
    void textColorChanged();
    void itemColorChanged();
    void lineColorChanged();
    void textButtonTriggered();
    void fillButtonTriggered();
    void lineButtonTriggered();
    void handleFontChange();
    void itemSelected(QGraphicsItem *item);
    void about();
    void openFileWithOdin();
    void setName();
    void findBlock();
    void selectAll();
    void showAllConnection();
    void resetHighlighting();
    void showNodeAndNeighboursOnly();
    void addParentsToHighlighting();
    void addChildrenToHighlighting();
    void showRelevantGraph();
    void expandCollapse();

private:
    void createToolBox();
    void createActions();
    void createMenus();
    void createToolbars();
    void configureClocks();

    ClockConfig clkconfig;

    QWidget *createBackgroundCellWidget(const QString &text,
                                        const QString &image);
    QWidget* createSimulationCellWidget(const QString &text,
                                        const QString &image);
    QWidget* createPowerCellWidget(const QString &text,
                                        const QString &image);
    QWidget *createCellWidget(const QString &text,
                              LogicUnit::UnitType type);
    QMenu *createColorMenu(const char *slot, QColor defaultColor);
    QIcon createColorToolButtonIcon(const QString &image, QColor color);
    QIcon createColorIcon(QColor color);

    ExplorerScene *scene;
    QGraphicsView *view;

    QAction *exitAction;
    QAction *addAction;
    QAction *deleteAction;
    QAction *setNameAction;
    QAction *selectAllAction;
    QAction *findAction;
    QAction *showAllConnectionsAction;
    QAction *removeHighlightingAction;
    QAction *showNodeAndNeighboursOnlyAction;
    QAction *addParentsToHighlightingAction;
    QAction *addChildrenToHighlightingAction;
    QAction *showRelevantGraphAction;
    QAction *expandCollapseAction;

    QAction *toFrontAction;
    QAction *sendBackAction;
    QAction *aboutAction;

    QAction *openFileAction;
    QAction *openFileOdinAction;

    QMenu *fileMenu;
    QMenu *itemMenu;
    QMenu *aboutMenu;

    QToolBar *openToolBar;
    QToolBar *textToolBar;
    QToolBar *editToolBar;
    QToolBar *colorToolBar;
    QToolBar *pointerToolbar;


    QComboBox *itemColorCombo;
    QComboBox *textColorCombo;
    QComboBox *fontSizeCombo;
    QFontComboBox *fontCombo;

    QToolBox *toolBox;
    QButtonGroup *nodeButtonGroup;
    QButtonGroup *pointerTypeGroup;
    QButtonGroup *backgroundButtonGroup;
    QButtonGroup* simulationButtonGroup;
    QButtonGroup* powerButtonGroup;
    QToolButton *fontColorToolButton;
    QToolButton *fillColorToolButton;
    QToolButton *lineColorToolButton;
    QAction *boldAction;
    QAction *underlineAction;
    QAction *italicAction;
    QAction *textAction;
    QAction *fillAction;
    QAction *lineAction;

    QString actBlifFilename;

    QGridLayout *simStatLayout;
    QLabel* simstatLabel;

    Container* myContainer;

    bool fileopen;
    int actSimStep, maxSimStep;
    int activityBase;
};
#endif
