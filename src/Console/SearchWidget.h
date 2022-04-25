/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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
#pragma once

#include <QTextEdit>
#include <QWidget>

class QLineEdit;
namespace FOEDAG {

class SearchWidget : public QWidget {
 public:
  SearchWidget(QTextEdit *searchEdit, QWidget *parent = nullptr,
               Qt::WindowFlags f = Qt::WindowFlags());

 public slots:
  void search();

 protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

 private slots:
  void findNext();

 private:
  QTextEdit *m_searchEdit{nullptr};
  QString m_textToSearch;
  bool m_enableSearch{false};
  QTextDocument::FindFlags m_searchFlags;
  QLineEdit *m_edit{nullptr};
};

}  // namespace FOEDAG
