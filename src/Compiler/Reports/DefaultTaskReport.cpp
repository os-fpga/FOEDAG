#include "DefaultTaskReport.h"

#include "IDataReport.h"

namespace FOEDAG {
DefaultTaskReport::DefaultTaskReport(ITaskReport::DataReports &&dataReports,
                                     const QString &name)
    : m_dataReports{std::move(dataReports)}, m_name{name} {}

const ITaskReport::DataReports &DefaultTaskReport::getDataReports() const {
  return m_dataReports;
}

const QString &DefaultTaskReport::getName() const { return m_name; }

}  // namespace FOEDAG
