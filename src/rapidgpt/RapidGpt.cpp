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

#include "RapidGpt.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

constexpr uint limit{32 * 1024};

// TODO unicode support

namespace FOEDAG {

RapidGpt::RapidGpt(const QString& key)
    : m_key(key), m_networkManager(new QNetworkAccessManager(this)) {
  connect(m_networkManager, &QNetworkAccessManager::finished, this,
          &RapidGpt::reply);
}

bool RapidGpt::send(RapidGptContext& context) {
  if (context.data.count() == 0) return false;
  if (m_key.isEmpty()) return false;
  m_context = &context;
  m_errorString.clear();
  QUrl url = QUrl(this->url());
  QNetworkRequest req(url);
  req.setRawHeader("Accept", "application/json");
  req.setRawHeader("Content-Type", "application/json");
  m_networkManager->post(req, BuildString(context).toLatin1());
  int res = m_eventLoop.exec();
  m_context = nullptr;
  return res == 0;
}

QString RapidGpt::errorString() const { return m_errorString; }

void RapidGpt::reply(QNetworkReply* r) {
  if (r->error() != QNetworkReply::NoError) {
    m_errorString = r->errorString();
    m_eventLoop.exit(1);
  } else {
    if (m_context) {
      QByteArray data = r->readAll();
      auto doc = QJsonDocument::fromJson(data);
      QJsonObject obj = doc.object();
      auto message = obj.find("message");
      if (message != obj.end()) {
        m_context->data.push_back(message->toString());
      }
    }
    m_eventLoop.exit(0);
  }
}

QString RapidGpt::url() const {
  return QString{
      "https://api.primis.ai/"
      "ask?api_key=%1&temperature=0.5&interactivity_rate=-1"}
      .arg(m_key);
}

QString RapidGpt::BuildString(const RapidGptContext& context) {
  QStringList objects{};
  for (int i = 0; i < context.data.size(); i++) {
    objects.push_back(QString{R"({"role":"%1","content":"%2"})"}.arg(
        (i % 2 == 0) ? "user" : "assistant",
        (context.data.at(i).size() >= limit) ? context.data.at(i).first(limit)
                                             : context.data.at(i)));
  }
  return QString{"[%1]"}.arg(objects.join(","));
}

}  // namespace FOEDAG
