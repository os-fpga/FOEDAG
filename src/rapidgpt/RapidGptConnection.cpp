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

#include "RapidGptConnection.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <chrono>

constexpr uint limit{32 * 1024};
using Time = std::chrono::high_resolution_clock;
using ms = std::chrono::milliseconds;

// TODO unicode support

namespace FOEDAG {

RapidGptConnection::RapidGptConnection(const RapidGptSettings& settings)
    : m_settings(settings), m_networkManager(new QNetworkAccessManager(this)) {
  connect(m_networkManager, &QNetworkAccessManager::finished, this,
          &RapidGptConnection::reply);
  m_networkManager->setTransferTimeout();  // 30 sec
}

bool RapidGptConnection::send(const RapidGptContext& context) {
  if (context.messages.count() == 0) return false;
  if (m_settings.key.isEmpty()) {
    m_errorString = "API Key is empty";
    return false;
  }
  if (!validateUserInput(context)) {
    m_errorString =
        "File size exceeds limit!\nThe selected file is too large. Please "
        "choose a file that is not bigger than 32kB.";
    return false;
  }
  m_errorString.clear();
  m_response.clear();
  QUrl url = QUrl(this->url());
  QNetworkRequest req(url);
  req.setRawHeader("Accept", "application/json");
  req.setRawHeader("Content-Type", "application/json");
  auto start = Time::now();
  m_networkManager->post(req, toByteArray(context));
  int res = m_eventLoop.exec();
  auto end = Time::now();
  auto fs = end - start;
  ms d = std::chrono::duration_cast<ms>(fs);
  m_delay = static_cast<double>(d.count()) / 1000.0;
  return res == 0;
}

QString RapidGptConnection::errorString() const { return m_errorString; }

QString RapidGptConnection::responseString() const { return m_response; }

double RapidGptConnection::delay() const { return m_delay; }

void RapidGptConnection::reply(QNetworkReply* r) {
  if (r->error() != QNetworkReply::NoError) {
    m_errorString = r->errorString();
    m_eventLoop.exit(1);
  } else {
    QByteArray data = r->readAll();
    auto doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    auto message = obj.find("message");
    if (message != obj.end()) m_response = message->toString();
    m_eventLoop.exit(0);
  }
}

QString RapidGptConnection::url() const {
  return QString{"%1/ask?api_key=%2&temperature=%3&interactivity_rate=%4"}.arg(
      m_settings.remoteUrl.isEmpty() ? "https://api.primis.ai"
                                     : m_settings.remoteUrl,
      m_settings.key, m_settings.precision, m_settings.interactive);
}

bool RapidGptConnection::validateUserInput(const RapidGptContext& context) {
  if (std::any_of(context.messages.begin(), context.messages.end(),
                  [](const Message& msg) {
                    return msg.content.toUtf8().size() >= limit;
                  }))
    return false;
  return true;
}

QByteArray RapidGptConnection::toByteArray(const RapidGptContext& context) {
  QJsonDocument doc;
  QJsonArray array;
  for (const auto& message : context.messages) {
    QJsonObject o;
    o.insert("role", (message.role == "User") ? "user" : "assistant");
    o.insert("content", message.content);
    array.append(o);
  }
  doc.setArray(array);
  return doc.toJson();
}

}  // namespace FOEDAG
