/*
    Copyright (c) 2016 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 3 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#pragma once

#include <QObject>
#include <QVector>

#include "utility.h"

class QProcess;

struct CMakeTarget
{
  enum TargetType { EXECUTABLE, STATIC_LIBRARY,
                    SHARED_LIBRARY, MODULE_LIBRARY,
                    OBJECT_LIBRARY, UTILITY, GLOBAL_TARGET,
                    INTERFACE_LIBRARY,
                    UNKNOWN_LIBRARY};
  QString Name;
  QString Path;
  QString ProjectName;
  QVector<QPair<QString, int> > Backtrace;
  TargetType Type;

  uint Line;

//#define DECLARE_MEMBER(NAME) QStringList NAME;
//  CMB_FOR_EACH_SOURCE_TYPE(DECLARE_MEMBER)
//  CMB_FOR_EACH_BUILD_PROPERTY_TYPE(DECLARE_MEMBER)
//#undef DECLARE_MEMBER

  static TargetType typeFromString(const QString& ts);
  static QString stringFromType(TargetType t);
};

inline bool operator==(const CMakeTarget& lhs, const CMakeTarget& rhs)
{
  return
//#define COMPARE_MEMBER(NAME) lhs.NAME == rhs.NAME &&
//  CMB_FOR_EACH_SOURCE_TYPE(COMPARE_MEMBER)
//  CMB_FOR_EACH_BUILD_PROPERTY_TYPE(COMPARE_MEMBER)
//#undef COMPARE_MEMBER
       lhs.Name == rhs.Name
    && lhs.Path == rhs.Path
    && lhs.Type == rhs.Type;
}

inline bool operator!=(const CMakeTarget& lhs, const CMakeTarget& rhs)
{
  return !(lhs == rhs);
}

enum TokenType {
  Command,
  UserCommand,
  Identifier,
  OpenParen,
  QuotedArgument,
  ClosedParen
};

struct Fragment
{
  int line;
  int column;
  int length;
  TokenType tokenType;
};

inline bool operator==(const Fragment& lhs, const Fragment& rhs)
{
  return lhs.line == rhs.line && lhs.column == rhs.column && lhs.length == rhs.length && lhs.tokenType == rhs.tokenType;
}

inline bool operator!=(const Fragment& lhs, const Fragment& rhs)
{
  return !(lhs == rhs);
}

class CMakeClient : public QObject
{
  Q_OBJECT
public:

  enum State {
    NotRunning,
    Initializing,
    Configuring,
    Computing,
    Idle
  };

  State GetState() const;

  QString buildDir() const;
  QString sourceDir() const;
  QString projectName() const;

  CMakeClient(QObject* parent = nullptr);

  void start(const QString& cmakeExe, const QString& buildDir);

  void retrieveTargets();
  void retrieveContent(long line, QString const& filePath,
                       const QString& fileContent);
  void retrieveDiffContent(long line1, QString const& filePath1,
                           long line2, QString const& filePath2,
                           const QString& fileContent);
  void retrieveParsed(QString const& filePath, const QString& content = {});
  void retrieveContextualHelp(const QString& filePath,
                              int line, int column,
                              const QString& fileContent);
  void retrieveSources(QString const& targetName);

  void retrieveCompletions(long line, long column,
                           QString const& filePath,
                           const QString& fileContent);

Q_SIGNALS:
  void errorReported();
  void stateChanged();

  void stdoutReceieved(const QString& stdoutContent);
  void stdinWritten(const QString& stdinContent);

  void targetsRetrieved(QStringList const& configs,
                        QVector<CMakeTarget> const& targetNames);
  void contentRetrieved(QMap<QString, QString> const& defMap);
  void diffContentRetrieved(QMap<QString, QString> const& addedMap,
                            QMap<QString, QString> const& removedMap);
  void parsedRetrieved(QMap<int, int> const& unreachableMap,
                       QVector<Fragment> const& fragments);
  void contextualHelpRetrieved(const QString& helpContext,
                               const QString& helpKey);
  void completionsRetrieved(const QString& matcher,
                            const QStringList& results,
                            const QStringList& descriptions);
  void sourcesRetrieved(const QString& tgtName,
                        const QStringList& srcs,
                        const QStringList& genSrcs);
  void includesRetrieved(const QStringList& incs);
  void definesRetrieved(const QStringList& incs);

  void sourceDirChanged();

private:
  void handleBuildsystemData(const QJsonObject& bs);
  void handleContent(const QJsonObject& bs);
  void handleDiffContent(const QJsonObject& bs);
  void handleParsed(const QJsonArray& unr, const QJsonArray& tok);
  void handleSources(const QJsonObject& tgtInfo);

  void makeRequest(QJsonObject const& obj);
  void writeHandshake();

private:
  QProcess* mServerProcess;
  QByteArray mDataBuffer;
  State mState;
  QString mBuildDir;
  QString mSourceDir;
  QString mProjectName;
};
