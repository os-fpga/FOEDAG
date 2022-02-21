#ifndef NEWFILEMODEL_H
#define NEWFILEMODEL_H

#include <QObject>

namespace FOEDAG {

class NewFileModel : public QObject {
  Q_OBJECT
 public:
  inline static const QStringList filters =
      QStringList() << "Verilog HDL Files(*.v)"
                    << "VHDL Files(*.vhd)"
                    << "Tcl Script Files(*.tcl)"
                    << "Synopsys Design Constraints Files(*.sdc)"
                    << "All Files(*.*)";

  explicit NewFileModel(QObject* parent = nullptr);

 signals:
  void openFile(QString);  // copy from new_file class. Not connected

 public slots:
  /**
   * @brief create new file with given name
   * @param fileName name of new file
   * @return bool true - if success or exist, false - if failed
   */
  bool createNewFile(const QString& fileName);
  /**
   * @brief create new file with given name and add extension if it wrong or
   * absent
   * @param fileName name of new file
   * @return bool true - if success or exist, false - if failed
   */
  bool createNewFileWithExtensionCheck(const QString& fileName,
                                       const QString& extension);
  /**
   * @brief set of filters for file dialog
   * @return list of file filters
   */
  QStringList fileDialogFilters();
};

}  // namespace FOEDAG

#endif  // NEWFILEMODEL_H
