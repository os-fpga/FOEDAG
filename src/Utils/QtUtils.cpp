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
#include "QtUtils.h"

#include <QRegularExpression>

namespace FOEDAG {

QStringList QtUtils::StringSplit(const QString &str, const QChar &sep) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  return str.split(sep, Qt::SkipEmptyParts);
#else
  return str.split(sep, QString::SkipEmptyParts);
#endif
}

bool QtUtils::IsEqual(const QString &str, const QString &s) {
  return str.compare(s, Qt::CaseInsensitive) == 0;
}

QString QtUtils::replaceTags(QString &data, const QStringList &tags) {
  const QString tagRegexpStr{
      "^<(?'tag'.*)>(\r\n|\r|\n)(?'content'\\X*)(\r\n|\r|\n)<\\/\\k{tag}>"};
  static const QRegularExpression tag{
      QString{"%1(\r\n|\r|\n)"}.arg(tagRegexpStr),
      QRegularExpression::MultilineOption |
          QRegularExpression::InvertedGreedinessOption};
  auto matches = tag.globalMatch(data);
  while (matches.hasNext()) {  // remove tags
    QRegularExpressionMatch match = matches.next();
    auto tagName = match.captured("tag");
    if (!tags.contains(tagName)) {
      data.replace(match.capturedStart(), match.capturedLength(), QString{});
      matches = tag.globalMatch(data, match.capturedStart());
    }
  }

  static const QRegularExpression existingTag{
      tagRegexpStr, QRegularExpression::MultilineOption |
                        QRegularExpression::InvertedGreedinessOption};
  matches = existingTag.globalMatch(data);
  while (matches.hasNext()) {  // replace tags with it's content
    QRegularExpressionMatch match = matches.next();
    QString newContent{match.captured("content")};
    data.replace(match.capturedStart(), match.capturedLength(), newContent);
    matches = existingTag.globalMatch(data);
  }
  return data;
}

std::string QtUtils::replaceTags(const std::string &data,
                                 const std::vector<std::string> &tags) {
  QString d = QString::fromStdString(data);
  QStringList tagsList{};
  for (const auto &t : tags) tagsList.push_back(QString::fromStdString(t));
  replaceTags(d, tagsList);
  return d.toStdString();
}

}  // namespace FOEDAG
