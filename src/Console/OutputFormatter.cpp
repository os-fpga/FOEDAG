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
#include "OutputFormatter.h"

#include <QDebug>
#include <QTextEdit>
#include <QtAlgorithms>

namespace FOEDAG {

Q_GLOBAL_STATIC_WITH_ARGS(QString, linkSep, {"::"})

OutputFormatter::OutputFormatter() {}

OutputFormatter::~OutputFormatter() { qDeleteAll(m_parsers); }

void OutputFormatter::appendMessage(const QString &message,
                                    OutputFormat format) {
  if (message.isEmpty()) return;
  if (!textEdit()) return;
  if (format < 0 || format >= Count) return;

  m_messageBuffer.append(message);
  int index = m_messageBuffer.indexOf('\n', Qt::CaseInsensitive);
  while (index != -1) {  // perform parsing line by line
    auto line = m_messageBuffer.mid(0, index + 1);

    LineParser::Status status{LineParser::Status::NotHandled};
    for (auto parser : m_parsers) {
      auto res = parser->handleLine(line, format);
      if (res.status == LineParser::Status::Done) {
        status = res.status;
        auto outputFormats = parseResults(line, format, res.linkSpecs);
        for (auto const &output : outputFormats) {
          textEdit()->textCursor().insertText(output.text, output.format);
        }
        break;  // break, when one of the parsers was success to parse line
      }
    }

    if (status == LineParser::Status::NotHandled) {
      textEdit()->textCursor().insertText(line, m_formats[format]);
    }

    m_messageBuffer.remove(0, index + 1);
    index = m_messageBuffer.indexOf('\n', Qt::CaseInsensitive);
  }
}

const std::vector<LineParser *> &OutputFormatter::parsers() const {
  return m_parsers;
}

void OutputFormatter::setParsers(const std::vector<LineParser *> &newParsers) {
  qDeleteAll(m_parsers);
  m_parsers = newParsers;
}

void OutputFormatter::addParser(LineParser *parser) {
  m_parsers.push_back(parser);
}

void OutputFormatter::initFormats() {
  m_formats[Regular].setForeground(Qt::black);
  m_formats[Output].setForeground(Qt::blue);
  m_formats[Error].setForeground(Qt::red);
  m_formats[Completion].setForeground(Qt::darkGreen);

#ifdef __APPLE__
  QFont f{"Monaco"};
#else
  QFont f{"Courier"};
#endif

  for (auto &format : m_formats) format.setFont(f);
}

OutputFormatter::FormattedTexts OutputFormatter::parseResults(
    const QString &text, OutputFormat format,
    const LineParser::LinkSpecs &links) const {
  FormattedTexts texts;
  int totalLength = 0;
  for (auto const &link : links) {
    QString t = text.mid(totalLength, link.startPos - totalLength);
    texts.push_back(FormattedText{t, m_formats[format]});
    totalLength += t.length();

    QString linked = text.mid(link.startPos, link.length);
    texts.push_back(
        FormattedText{linked, linkedText(m_formats[format], link.href)});
    totalLength += link.length;
  }
  if (totalLength < text.length()) {
    texts.push_back(FormattedText{text.mid(totalLength), m_formats[format]});
  }
  return texts;
}

QTextCharFormat OutputFormatter::linkedText(const QTextCharFormat &inputFormat,
                                            const QString &href) {
  QTextCharFormat linked = inputFormat;
  linked.setAnchor(true);
  linked.setAnchorHref(href);
  linked.setUnderlineStyle(QTextCharFormat::SingleUnderline);
  linked.setForeground(Qt::blue);
  return linked;
}

QTextEdit *OutputFormatter::textEdit() const { return m_textEdit; }

void OutputFormatter::setTextEdit(QTextEdit *newTextEdit) {
  m_textEdit = newTextEdit;
  initFormats();
}

LineParser::~LineParser() {}

QString addLinkSpecForAbsoluteFilePath(const QString filePath,
                                       const QString &line) {
  return filePath + *linkSep() + line;
}

}  // namespace FOEDAG
