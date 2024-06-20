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
#include "FileExplorer.h"

#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QTreeView>
#include <filesystem>

namespace FOEDAG {

class FileSystemModel : public QFileSystemModel {
 public:
  int columnCount(const QModelIndex &parent) const override {
    return (parent.column() > 0) ? 0 : 1;
  }
  QVariant data(const QModelIndex &index, int role) const override {
    if (role == Qt::ToolTipRole) {
      if (data(index, Qt::DisplayRole).toString() == "..") {
        return fileInfo(index).dir().canonicalPath();
      }
    }
    return QFileSystemModel::data(index, role);
  }
};

FileExplorer::FileExplorer(QObject *parent) : QObject(parent) {
  m_model = new FileSystemModel;
  m_model->setIconProvider(new QFileIconProvider);
  m_model->setRootPath({});
  m_model->setFilter(QDir::AllEntries | QDir::Files | QDir::Filter::NoDot);
  m_tree = new QTreeView;
  connect(m_tree, &QTreeView::doubleClicked, this,
          [this](const QModelIndex &index) {
            auto path = m_model->filePath(index);
            if (std::filesystem::is_regular_file(path.toStdString())) {
              emit openFile(path);
            } else {
              setRootPath(path);
            }
          });
  m_tree->setModel(m_model);
}

void FileExplorer::setRootPath(const QString &path) {
  m_tree->setRootIndex(m_model->index(path));
}

QWidget *FileExplorer::widget() { return m_tree; }

QString FileExplorer::canonicalPath(const QString &path) {
  return QString::fromStdString(
      std::filesystem::canonical(std::filesystem::path{path.toStdString()})
          .string());
}

}  // namespace FOEDAG
