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

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

#include "ChatWidget.h"
#include "RapidGptConnection.h"
#include "Utils/FileUtils.h"
#include "Utils/QtUtils.h"

static const char *chat_rapidgpt{"chat.rapidgpt"};
static const char *User{"User"};
static const char *RapidGPT{"RapidGPT"};

namespace FOEDAG {

RapidGpt::RapidGpt(const RapidGptSettings &settings,
                   const std::filesystem::path &projectPath, QObject *parent)
    : QObject(parent),
      m_chatWidget(new ChatWidget),
      m_path(projectPath),
      m_settings(settings) {
  connect(m_chatWidget, &ChatWidget::userText, this, &RapidGpt::sendUserText);
  connect(m_chatWidget, &ChatWidget::cleanHistory, this,
          &RapidGpt::cleanCurrentHistory);
  connect(m_chatWidget, &ChatWidget::regenerateLast, this,
          &RapidGpt::regenerateLast);
  connect(m_chatWidget, &ChatWidget::removeMessageAt, this,
          &RapidGpt::removeMessageAt);
  loadFromFile();
}

QWidget *RapidGpt::widget() { return m_chatWidget; }

void RapidGpt::setSettings(const RapidGptSettings &settings) {
  m_settings = settings;
}

void RapidGpt::setProjectPath(const std::filesystem::path &projectPath) {
  m_path = projectPath;
  loadFromFile();
}

void RapidGpt::fileContext(const QString &file) {
  m_chatWidget->clear();
  m_chatWidget->setEnableIncognitoMode(file.isEmpty());
  RapidGptContext context;
  m_currectFile = file;
  if (!m_files.contains(file))
    m_files[file] = context;
  else
    context = m_files.value(file);
  updateChat(context);
}

void RapidGpt::flush() {
  if (FileUtils::FileExists(m_path)) {
    QString filePath =
        QString::fromStdString((m_path / chat_rapidgpt).string());
    QFile file{filePath};
    QJsonDocument doc;
    QJsonObject mainObject{};
    for (const auto &[openedFile, context] : QU::asKeyValueRange(m_files)) {
      if (openedFile.isEmpty()) continue;
      if (context.messages.isEmpty()) continue;
      std::filesystem::path p = std::filesystem::path{openedFile.toStdString()};
      auto relative_p = std::filesystem::relative(p, m_path);
      QJsonArray array;
      for (const auto &entry : context.messages) {
        QJsonObject o;
        o.insert("role", (entry.role == User) ? "user" : "assistant");
        o.insert("content", entry.content);
        o.insert("date", entry.date);
        o.insert("delay", entry.delay);
        array.append(o);
      }
      QJsonObject tmpObj;
      tmpObj[QString::fromStdString(relative_p.string()) + "+general"] = array;
      mainObject[QString::fromStdString(relative_p.string())] = tmpObj;
    }
    doc.setObject(mainObject);
    if (file.open(QFile::WriteOnly)) {
      file.write(doc.toJson());
    }
  }
}

void RapidGpt::sendUserText(const QString &text) {
  m_chatWidget->addMessage({text, User, currentDate(), 0.0});
  sendRapidGpt(text);
}

void RapidGpt::cleanCurrentHistory() {
  if (isIncognitoMode()) {
    auto answer =
        QMessageBox::question(m_chatWidget, "Delete chat history",
                              "Do you want to delete the chat history?");
    if (answer == QMessageBox::Yes) {
      m_chatWidget->clear();
      m_files.remove(m_currectFile);
    }
  } else {
    auto answer = QMessageBox::question(
        m_chatWidget, "Delete chat history",
        QString{"Do you want to delete the chat history for the file %1?"}.arg(
            QFileInfo{m_currectFile}.fileName()));
    if (answer == QMessageBox::Yes) {
      m_chatWidget->clear();
      m_files.remove(m_currectFile);
      flush();
    }
  }
}

void RapidGpt::regenerateLast() {
  if (!m_files[m_currectFile].messages.isEmpty()) {
    m_chatWidget->setEnableToSend(false);
    if (m_files[m_currectFile].messages.last().role != User) {
      m_chatWidget->removeAt(m_chatWidget->count() - 1);
      m_files[m_currectFile].messages.takeLast();
      flush();
    }
    auto lastUserMessage = m_files[m_currectFile].messages.takeLast();
    sendRapidGpt(lastUserMessage.content);
  }
}

void RapidGpt::removeMessageAt(int index) {
  // need to remove user message and rapidgpt message
  if (index >= 0 && index < m_files[m_currectFile].messages.count()) {
    int itemsToRemove =
        ((index + 1) == m_files[m_currectFile].messages.count()) ? 1 : 2;
    m_files[m_currectFile].messages.remove(index, itemsToRemove);
    m_chatWidget->removeAt(index);
    if (itemsToRemove > 1) m_chatWidget->removeAt(index);
    flush();
  }
}

QString RapidGpt::GetFileContent() const {
  if (!isIncognitoMode()) {
    QFile file{m_currectFile};
    if (file.open(QFile::ReadOnly)) return file.readAll();
  }
  return {};
}

QString RapidGpt::currentDate() {
  return QDateTime::currentDateTime().toString("MM/dd/yyyy, hh:mm:ss");
}

void RapidGpt::updateChat(const RapidGptContext &context) {
  for (auto &message : context.messages) {
    m_chatWidget->addMessage(message);
  }
}

void RapidGpt::loadFromFile() {
  if (FileUtils::FileExists(m_path)) {
    QString filePath =
        QString::fromStdString((m_path / chat_rapidgpt).string());
    QFile file{filePath};
    if (file.open(QFile::ReadOnly)) {
      QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
      auto mainObject = doc.object();
      for (auto it = mainObject.begin(); it != mainObject.end(); ++it) {
        QJsonObject internal = it.value().toObject();
        auto keys = internal.keys();
        if (!keys.isEmpty()) {
          QJsonArray array = internal.value(keys.at(0)).toArray();
          RapidGptContext contex;
          for (const auto &arrayEntry : array) {
            QJsonObject messageObj = arrayEntry.toObject();
            contex.messages.push_back(
                {messageObj.value("content").toString(),
                 messageObj.value("role").toString() == "user" ? User
                                                               : RapidGPT,
                 messageObj.value("date").toString(),
                 messageObj.value("delay").toDouble()});
          }
          m_files[buildPath(it.key())] = contex;
        }
      }
    }
  }
}

QString RapidGpt::buildPath(const QString &relativePath) const {
  std::filesystem::path path =
      m_path / std::filesystem::path{relativePath.toStdString()};
  std::error_code errorCode{};
  path = std::filesystem::canonical(path, errorCode);
  return QString::fromStdString(path.string());
}

bool RapidGpt::sendRapidGpt(const QString &text) {
  auto result{true};
  m_errorString.clear();
  m_files[m_currectFile].messages.append({text, User, currentDate(), 0.0});
  RapidGptConnection rapidGpt{m_settings};
  RapidGptContext tmpContext = compileContext();
  bool ok = rapidGpt.send(tmpContext);
  if (ok) {
    auto res = rapidGpt.responseString();
    Message m = {res, RapidGPT, currentDate(), rapidGpt.delay()};
    m_chatWidget->addMessage(m);
    m_files[m_currectFile].messages.push_back(m);
    flush();
  } else {
    m_errorString = rapidGpt.errorString();
    if (m_showError)
      QMessageBox::critical(m_chatWidget, "Error", m_errorString);
    result = false;
  }
  m_chatWidget->setEnableToSend(true);
  return result;
}

QString RapidGpt::errorString() const { return m_errorString; }

void RapidGpt::setShowError(bool showError) { m_showError = showError; }

RapidGptContext RapidGpt::compileContext() const {
  auto context = m_files.value(m_currectFile, RapidGptContext{});
  if (!context.messages.isEmpty()) {
    context.messages[0].content =
        QString("The context you operate in is the following:\n%1\n%2")
            .arg(GetFileContent(), context.messages.at(0).content);
  }
  return context;
}

bool RapidGpt::isIncognitoMode() const { return m_currectFile.isEmpty(); }

}  // namespace FOEDAG
