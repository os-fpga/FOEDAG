/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "ReportGenerator.h"

#include <QBoxLayout>
#include <QFile>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QTextStream>

#include "Compiler/Reports/IDataReport.h"

namespace FOEDAG {

QLabel* createTitleLabel(const QString& text) {
  auto titleLabel = new QLabel(text);
  auto font = titleLabel->font();
  font.setBold(true);
  titleLabel->setFont(font);

  return titleLabel;
}

ReportGenerator::ReportGenerator(const ITaskReport& report)
    : m_report(report) {}

ReportGenerator::~ReportGenerator() {}

TableReportGenerator::TableReportGenerator(const ITaskReport& report,
                                           QBoxLayout* layout)
    : ReportGenerator(report), m_layout(layout) {}

void TableReportGenerator::Generate() {
  QVector<QTableWidget*> views{};
  for (auto& dataReport : m_report.getDataReports()) {
    auto dataReportName = dataReport->getName();
    if (!dataReportName.isEmpty())
      m_layout->addWidget(createTitleLabel(dataReportName));

    if (dataReport->isEmpty()) {
      m_layout->addWidget(
          new QLabel("No statistics data found to generate report."), 1,
          Qt::AlignTop);
      continue;
    }
    auto reportsView = new QTableWidget();
    reportsView->verticalHeader()->hide();
    // Fill columns
    auto columns = dataReport->getColumns();
    reportsView->setColumnCount(columns.size());
    auto colIndex = 0;
    for (auto& col : columns) {
      auto columnItem = new QTableWidgetItem(col.m_name);
      reportsView->setHorizontalHeaderItem(colIndex, columnItem);
      ++colIndex;
    }

    // Fill table
    auto rowIndex = 0;
    for (auto& lineData : dataReport->getData()) {
      reportsView->insertRow(rowIndex);
      auto colIndex = 0;
      for (auto& lineValue : lineData) {
        auto item = new QTableWidgetItem(lineValue);
        item->setTextAlignment(columns[colIndex].m_alignment);
        reportsView->setItem(rowIndex, colIndex, item);
        ++colIndex;
      }
      ++rowIndex;
    }
    // Initialize the view itself
    reportsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    reportsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    reportsView->horizontalHeader()->resizeSections(
        QHeaderView::ResizeToContents);
    m_layout->addWidget(reportsView);
    views.push_back(reportsView);
  }
  if (m_layout->count() != 0) {
    // add spacer item at the end of the layout
    m_layout->addStretch(20);
  }
  // align first column for all tables
  int maxWidth{0};
  for (auto view : views) {
    maxWidth = std::max(maxWidth, view->columnWidth(0));
  }
  for (auto view : views) {
    view->setColumnWidth(0, maxWidth);
  }
}

FileReportGenerator::FileReportGenerator(const ITaskReport& report,
                                         const QString& fileName)
    : ReportGenerator(report), m_fileName(fileName) {}

void FileReportGenerator::Generate() {
  // TODO dump to file exmaple
  QFile file{m_fileName};
  file.open(QFile::WriteOnly | QFile::Text);
  const auto newLine{"\n"};
  QTextStream data{&file};
  for (auto& dataReport : m_report.getDataReports()) {
    auto dataReportName = dataReport->getName();
    data << dataReportName << newLine;
    for (auto& lineData : dataReport->getData()) {
      for (auto& lineValue : lineData) {
        for (auto lv : lineValue) data << lv;
        data << " ";
      }
      data << newLine;
    }
  }
  file.close();
}

}  // namespace FOEDAG
