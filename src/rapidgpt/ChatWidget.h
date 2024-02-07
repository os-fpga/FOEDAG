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
#pragma once

#include <QWidget>

#include "RapidGptContext.h"

namespace Ui {
class ChatWidget;
}

class QSpacerItem;

namespace FOEDAG {

class MessageOutput;
class ChatWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ChatWidget(QWidget *parent = nullptr);
  ~ChatWidget() override;
  void addMessage(const Message &message);
  void clear();

  int count() const;
  void removeAt(int index);
  void setEnableToSend(bool enable);

  void setEnableIncognitoMode(bool enable);

 signals:
  void userText(const QString &text);
  void cleanHistory();
  void regenerateLast();
  void removeMessageAt(int index);

 protected:
  void keyPressEvent(QKeyEvent *event) override;

 private slots:
  void buttonClicked();
  void openLink(const QString &link);

 private:
  void updateMessageButtons();
  void editMessage(int index);

 private:
  Ui::ChatWidget *ui;
  QVector<MessageOutput *> m_widgets{};
  bool m_enable{true};
  QSpacerItem *m_spacer{};
};

}  // namespace FOEDAG
