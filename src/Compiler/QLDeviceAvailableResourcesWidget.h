#pragma once

#include <QWidget>
#include <QLabel>
#include <QMovie>

#include <optional>

class QShowEvent;

namespace FOEDAG {

class QLDeviceAvailableResourcesWidget final: public QWidget {
  Q_OBJECT
 public:
  QLDeviceAvailableResourcesWidget(QWidget* parent = nullptr);
  ~QLDeviceAvailableResourcesWidget() override final = default;

  void setDevicevariantKey(const QString& deviceVariantKey);
  const QString& deviceVariantKey() const { return m_deviceVariantKey; }

  void showValues(const std::optional<int>& bram, const std::optional<int>& dsp, const std::optional<int>& clb);

 protected:
  void showEvent(QShowEvent*) override final;

 private:
  QString m_deviceVariantKey;
  QLabel* m_label;
  QLabel* m_label_progress;
  QMovie* m_movie_progress;

  void showProgress();
  void hideProgress();
};

}  // namespace FOEDAG


