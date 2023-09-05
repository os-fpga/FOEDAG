#include "QLDeviceAvailableResourcesWidget.h"

#include <QHBoxLayout>

namespace FOEDAG {

QLDeviceAvailableResourcesWidget::QLDeviceAvailableResourcesWidget(QWidget* parent): QWidget(parent)
{
  m_label = new QLabel();
  m_label_progress = new QLabel();
  m_movie_progress = new QMovie(":/loading.gif", {}, this);
  m_label_progress->setFixedSize(20, 20);
  m_label_progress->setMovie(m_movie_progress);

  QHBoxLayout* layout = new QHBoxLayout(this);
  setLayout(layout);
  layout->setContentsMargins(0,0,0,0);

  layout->addWidget(m_label_progress);
  layout->addWidget(m_label);
  layout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
}

void QLDeviceAvailableResourcesWidget::reset()
{
  m_label->setText("");
  m_label->setVisible(false);
}

void QLDeviceAvailableResourcesWidget::showProgress()
{
  m_label_progress->setVisible(true);
  m_movie_progress->start();
}

void QLDeviceAvailableResourcesWidget::hideProgress()
{
  m_label_progress->setVisible(false);
  m_movie_progress->stop();
}

void QLDeviceAvailableResourcesWidget::showValues(const QString& layout, const std::optional<int>& bram, const std::optional<int>& dsp, const std::optional<int>& clb)
{
  m_layout = layout;
  QString archInfo;
  if (bram) {
    archInfo += "bram: <b>" + QString::number(bram.value()) + " </b>";
  }
  if (dsp) {
    archInfo += "dsp: <b>" + QString::number(dsp.value()) + " </b>";
  }
  if (clb) {
    archInfo += "clb: <b>" + QString::number(clb.value()) + " </b>";
  }

  if (!archInfo.isEmpty()) {
    hideProgress();
    m_label->setVisible(true);
    m_label->setText(archInfo);
  } else {
    m_label->setVisible(false);
  }
}

}  // namespace FOEDAG
