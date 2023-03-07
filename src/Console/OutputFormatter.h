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

#include <QTextCharFormat>
#include <vector>

class QTextEdit;

namespace FOEDAG {

enum OutputFormat {
  Regular = 0,
  Output = 1,
  Error = 2,
  Completion = 3,
  Count = 4,  // should be the last
};

QString addLinkSpecForAbsoluteFilePath(const QString filePath,
                                       const QString &line);

class LineParser {
 public:
  virtual ~LineParser();
  enum class Status { Done, NotHandled };
  class LinkSpec {
   public:
    LinkSpec(int sp, int l, const QString &hr)
        : startPos(sp), length(l), href(hr) {}
    int startPos = -1;
    int length = -1;
    QString href;
  };
  using LinkSpecs = std::vector<LinkSpec>;
  class Result {
   public:
    Result(Status s, const QString &t = QString(), const LinkSpecs &specs = {})
        : linkSpecs(specs), text(t), status(s) {}
    LinkSpecs linkSpecs;
    QString text;
    Status status;
  };

  virtual Result handleLine(const QString &message, OutputFormat format) = 0;
};

class FormattedText {
 public:
  FormattedText(const QString &t, const QTextCharFormat &f)
      : text(t), format(f) {}
  QString text;
  QTextCharFormat format;
};

class OutputFormatter {
  using FormattedTexts = std::vector<FormattedText>;

 public:
  OutputFormatter();
  ~OutputFormatter();
  void appendMessage(const QString &message, OutputFormat format);

  const std::vector<LineParser *> &parsers() const;
  /*!
   * \brief setParsers. It takes the ownership of the pointers
   */
  void setParsers(const std::vector<LineParser *> &newParsers);

  /*!
   * \brief addParser. Add \param parser to the list and take ownership of the
   * pointer
   */
  void addParser(LineParser *parser);

  void setTextEdit(QTextEdit *newTextEdit);
  QTextEdit *textEdit() const;

 private:
  void initFormats();
  FormattedTexts parseResults(const QString &text, OutputFormat format,
                              const LineParser::LinkSpecs &links) const;
  static QTextCharFormat linkedText(const QTextCharFormat &inputFormat,
                                    const QString &href);

 private:
  std::vector<LineParser *> m_parsers;
  std::vector<QTextCharFormat> m_formats{Count};
  QTextEdit *m_textEdit;
  QString m_messageBuffer;
};
}  // namespace FOEDAG
