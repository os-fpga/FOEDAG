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

#include <QEventLoop>

#include "RapidGptContext.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace FOEDAG {

class RapidGpt : public QObject {
  Q_OBJECT

 public:
  explicit RapidGpt(const QString &key);

  bool send(RapidGptContext &context);
  QString errorString() const;

 private slots:
  void reply(QNetworkReply *r);

 private:
  QString url() const;
  static QString BuildString(const RapidGptContext &context);

 private:
  QString m_key{};
  QNetworkAccessManager *m_networkManager{nullptr};
  QEventLoop m_eventLoop;
  RapidGptContext *m_context{nullptr};
  QString m_errorString{};
};

}  // namespace FOEDAG
