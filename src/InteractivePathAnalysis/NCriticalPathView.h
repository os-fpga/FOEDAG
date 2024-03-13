/**
  * @file NCriticalPathView.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
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

#pragma once

#include <QTreeView>

class QPushButton;
class QCheckBox;
class QMouseEvent;

namespace FOEDAG {

class CustomMenu;
class NCriticalPathFilterWidget;
class NCriticalPathItem;
class NCriticalPathModel;
class NCriticalPathFilterModel;
class FilterCriteriaConf;

class NCriticalPathView final : public QTreeView {
  Q_OBJECT

 public:
  explicit NCriticalPathView(QWidget* parent = nullptr);
  ~NCriticalPathView() override final = default;

  void clear();

 protected:
  void resizeEvent(QResizeEvent*) override final;
  void showEvent(QShowEvent*) override final;
  void mousePressEvent(QMouseEvent* event) override final;
  void mouseReleaseEvent(QMouseEvent* event) override final;
  void keyPressEvent(QKeyEvent* event) override final;

  void handleSelectionChanged(const QItemSelection& selected,
                              const QItemSelection& deselected);

 signals:
  void pathElementSelectionChanged(const QString&, const QString&);
  void loadFromString(const QString&);
  void dataLoaded();
  void dataCleared();

 private:
  int m_scrollStep = 10;
  QList<std::pair<QModelIndex, bool>>
      m_pathSourceIndexesToResolveChildrenSelection;
  bool m_isClearAllSelectionsPending = false;

  QString m_lastSelectedPathId;
  QPushButton* m_bnExpandCollapse = nullptr;
  bool m_isCollapsed = true;
  QPushButton* m_bnClearSelection = nullptr;

  QPushButton* m_bnFilter = nullptr;
  CustomMenu* m_filterMenu = nullptr;

  NCriticalPathFilterWidget* m_inputFilter = nullptr;
  NCriticalPathFilterWidget* m_outputFilter = nullptr;

  NCriticalPathModel* m_sourceModel = nullptr;
  NCriticalPathFilterModel* m_filterModel = nullptr;

  void setupFilterMenu();

  void updateControlsLocation();
  void hideControls();

  QString getSelectedPathElements() const;
  void updateChildrenSelectionFor(const QModelIndex& sourcePathIndex,
                                  bool selected) const;
  void scroll(int steps);

  void clearSelection();
  void fillInputOutputData(const std::map<QString, int>&,
                           const std::map<QString, int>&);

  void setupModels();
  void onActualDataLoaded();
  void onActualDataCleared();
};

}  // namespace FOEDAG