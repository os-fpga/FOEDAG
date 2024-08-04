#pragma once

#include <QAbstractTableModel>
#include <QVector>

namespace FOEDAG {

class ErrorsModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum COLUMN {
        ERROR=0,
        LINE_NUM=1,
        LINE=2
    };

    ErrorsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override final;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override final;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override final;

public slots:
    void setData(const QVector<QVector<QString>>& data);
    
private:
    QVector<QVector<QString>> m_data;
};

} // namespace FOEDAG