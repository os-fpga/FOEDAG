#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

#include <optional>

namespace FOEDAG {

class QLDeviceAvailableResourcesWidget : public QWidget {
  Q_OBJECT
 public:
  QLDeviceAvailableResourcesWidget(QWidget* parent = nullptr);
  ~QLDeviceAvailableResourcesWidget() = default;

  const QString& layout() const { return m_layout; }

  void reset();
  void showProgress();
  void hideProgress();
  void showValues(const QString& layout, const std::optional<int>& bram, const std::optional<int>& dsp, const std::optional<int>& clb);

 private:
  QString m_layout;
  QLabel* m_label;
  QLabel* m_label_progress;
  QMovie* m_movie_progress;
};

}  // namespace FOEDAG


