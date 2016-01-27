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

#include "cmakeclient.h"

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#define MAGIC_START "\n[== CMake MetaMagic ==[\n"
#define MAGIC_END "\n]== CMake MetaMagic ==]\n"

CMakeClient::CMakeClient(QObject* parent)
  : QObject(parent), mServerProcess(nullptr)
{
  mState = NotRunning;
}

CMakeClient::State CMakeClient::GetState() const
{
  return mState;
}

void CMakeClient::handleContent(QJsonObject const& bs)
{
  QMap<QString, QString> map;

  for(auto jsIt = bs.begin(); jsIt != bs.end(); ++jsIt)
    {
      Q_ASSERT(!jsIt.key().isEmpty());
      map[jsIt.key()] = jsIt.value().toString();
    }

  Q_EMIT contentRetrieved(map);
}

void CMakeClient::handleDiffContent(QJsonObject const& bs)
{
  QMap<QString, QString> removed;
  QMap<QString, QString> added;

  auto removedJs = bs["removedDefs"].toArray();
  auto addedJs = bs["addedDefs"].toArray();

  for(auto jsIt = removedJs.begin(); jsIt != removedJs.end(); ++jsIt)
    {
      auto obj = jsIt->toObject();
      Q_ASSERT(!obj["key"].toString().isEmpty());
      removed[obj["key"].toString()] = obj["value"].toString();
    }
  for(auto jsIt = addedJs.begin(); jsIt != addedJs.end(); ++jsIt)
    {
      auto obj = jsIt->toObject();
      Q_ASSERT(!obj["key"].toString().isEmpty());
      added[obj["key"].toString()] = obj["value"].toString();
    }
  Q_EMIT diffContentRetrieved(added, removed);
}

void CMakeClient::handleParsed(const QJsonArray& unr, const QJsonArray& tok)
{
  QMap<int, int> unrMap;
  QVector<Fragment> fragments;

  for(auto jsIt = unr.begin(); jsIt != unr.end(); ++jsIt)
    {
      Q_ASSERT(jsIt->isObject());
      auto obj = jsIt->toObject();
      unrMap[obj["begin"].toInt()] = obj["end"].toInt();
    }

  foreach(auto val, tok) {
    Fragment fragment;
    auto obj = val.toObject();
    fragment.line = obj["line"].toInt();
    fragment.column = obj["column"].toInt();
    fragment.length = obj["length"].toInt();
    QString type = obj["type"].toString();
    if (type == "command")
      fragment.tokenType = Command;
    else if (type == "user_command")
      fragment.tokenType = UserCommand;
    else if (type == "identifier")
      fragment.tokenType = Identifier;
    else if (type == "quoted argument") {
      fragment.tokenType = QuotedArgument;
      fragment.length += 2;
    }
    else if (type == "unquoted argument")
      fragment.tokenType = Identifier;
    else if (type == "left paren")
      fragment.tokenType = OpenParen;
    else if (type == "right paren")
      fragment.tokenType = ClosedParen;

    fragments.push_back(fragment);
  }

  Q_EMIT parsedRetrieved(unrMap, fragments);
}

void CMakeClient::handleSources(const QJsonObject& tgtInfo)
{
  auto srcsJS = tgtInfo["object_sources"].toArray();
  auto genSrcsJS = tgtInfo["generated_object_sources"].toArray();
  auto incsJS = tgtInfo["include_directories"].toArray();
  auto defsJS = tgtInfo["compile_definitions"].toArray();

  QStringList srcs;
  QStringList genSrcs;
  QStringList incs;
  QStringList defs;

  for (auto src: srcsJS)
    {
      srcs.push_back(src.toString());
    }

  for (auto src: genSrcsJS)
    {
      genSrcs.push_back(src.toString());
    }

  for (auto inc: incsJS)
    {
      incs.push_back(inc.toString());
    }

  for (auto def: defsJS)
    {
      defs.push_back(def.toString());
    }

  auto tgtName = tgtInfo["target_name"].toString();
  Q_EMIT sourcesRetrieved(tgtName, srcs, genSrcs);
  Q_EMIT includesRetrieved(incs);
  Q_EMIT definesRetrieved(defs);
}

void CMakeClient::handleBuildsystemData(QJsonObject const& bs)
{
  auto jsonConfigs = bs["configs"].toArray();
  QStringList configs;
  foreach(auto jsC, jsonConfigs)
    {
      configs.push_back(jsC.toString());
    }
  auto jsonTargets = bs["targets"].toArray();
  QVector<CMakeTarget> targets;
  foreach(auto jsT, jsonTargets)
    {
      QJsonObject jsTO = jsT.toObject();
      CMakeTarget tgt;
      tgt.Name = jsTO["name"].toString();
      tgt.ProjectName = jsTO["projectName"].toString();
      tgt.Type = CMakeTarget::typeFromString(jsTO["type"].toString());
      QVector<QPair<QString, int> > bt;
      foreach (auto bti, jsTO["backtrace"].toArray())
        {
          auto btFrame = bti.toObject();
          QPair<QString, int> frame(
                btFrame["path"].toString(),
              btFrame["line"].toInt());
          bt.push_back(frame);
        }
      std::reverse(bt.begin(), bt.end());
      tgt.Backtrace = bt;

      targets.push_back(tgt);
    }
  Q_EMIT targetsRetrieved(configs, targets);
}

QString CMakeClient::buildDir() const
{
  return mBuildDir;
}

QString CMakeClient::sourceDir() const
{
  return mSourceDir;
}

QString CMakeClient::projectName() const
{
  return mProjectName;
}

void CMakeClient::start(QString const& cmakeExe, QString const& buildDir)
{
  if (mServerProcess)
  {
    qDebug() << "TERM OLD" << mBuildDir;
    delete mServerProcess;
  }

  qDebug() << "START" << buildDir;
  mBuildDir = buildDir;
  mServerProcess = new QProcess(this);

  auto handleServerData = [this](){
      Q_FOREVER {
        int startPoint = mDataBuffer.indexOf(MAGIC_START);
        int endPoint = mDataBuffer.indexOf(MAGIC_END, startPoint);
        if (startPoint == -1 || endPoint == -1)
        {
          return;
        }
        startPoint += sizeof(MAGIC_START) - 1;
        auto jsonData = mDataBuffer.mid(startPoint, endPoint - startPoint);
        mDataBuffer = mDataBuffer.right(mDataBuffer.size() - endPoint - sizeof(MAGIC_END) + 1);

        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

        if (!jsonDoc.isObject()) {
          return;
          }

        QJsonObject obj = jsonDoc.object();

        if (obj.contains("progress"))
          {
            QString prog = obj.value("progress").toString();
            if (prog == "process-started")
              {
                mState = Initializing;
                emit stateChanged();
                writeHandshake();
              }
            if (prog == "idle")
              {
                mState = Idle;
                mSourceDir = obj.value("source_dir").toString();
                mProjectName = obj.value("project_name").toString();
                if (mBuildDir != obj.value("binary_dir").toString())
                  {
                    qDebug() << mBuildDir << obj.value("binary_dir").toString();
                  }
                Q_ASSERT(mBuildDir == obj.value("binary_dir").toString());
                emit stateChanged();
                // Need a message queue?
              }
          }
        else if (obj.contains("buildsystem"))
          {
            auto bs = obj["buildsystem"].toObject();
            handleBuildsystemData(bs);
          }
        else if (obj.contains("content"))
          {
            auto bs = obj["content"].toObject();
            handleContent(bs);
          }
        else if (obj.contains("content_result"))
          {
            auto bs = obj["content_result"].toString();
            if (bs == "unexecuted")
              {
                handleContent({});
              }
          }
        else if (obj.contains("content_diff"))
          {
            auto bs = obj["content_diff"].toObject();
            handleDiffContent(bs);
          }
        else if (obj.contains("target_info"))
          {
            auto bs = obj["target_info"].toObject();
            handleSources(bs);
          }
        else if (obj.contains("parsed"))
          {
            auto parsed = obj["parsed"].toObject();
            auto unr = parsed["unreachable"].toArray();
            auto tokens = parsed["tokens"].toArray();
            handleParsed(unr, tokens);
          }
        else if (obj.contains("contextual_help"))
          {
            auto unr = obj["contextual_help"].toObject();
            if (!unr.contains("nocontext")) {
                Q_EMIT contextualHelpRetrieved(
                      unr["context"].toString(),
                    unr["help_key"].toString());
              }
          }
        else if (obj.contains("completion"))
          {
            auto unr = obj["completion"].toObject();
            if (unr.contains("result") && unr["result"] == "no_completions")
              {
                qDebug() << "NO COMPLETIONS";
                return;
              }
            QString matcher = unr["matcher"].toString();
            QJsonArray results;
            if (unr.contains("targets"))
              {
                results = unr["targets"].toArray();
              }
            if (unr.contains("commands"))
              {
                results = unr["commands"].toArray();
              }
            if (unr.contains("variables"))
              {
                results = unr["variables"].toArray();
              }
            if (unr.contains("packages"))
              {
                results = unr["packages"].toArray();
              }
            if (unr.contains("modules"))
              {
                results = unr["modules"].toArray();
              }
            if (unr.contains("policies"))
              {
                results = unr["policies"].toArray();
              }
            if (unr.contains("keywords"))
              {
                results = unr["keywords"].toArray();
              }

            QStringList descriptions;
            if (unr.contains("descriptions"))
              {
              auto descr = unr["descriptions"].toArray();
              foreach(auto res, descr)
                {
                descriptions.push_back(res.toString());
                }
              }
            QStringList strings;
            foreach(auto res, results)
              {
                strings.push_back(res.toString());
              }
            completionsRetrieved(matcher, strings, descriptions);
          }
        }
    };

  connect(mServerProcess, &QProcess::readyReadStandardOutput, [this, handleServerData] {
      auto newBit = mServerProcess->readAll();
      mDataBuffer += newBit;
      Q_EMIT stdoutReceieved(newBit);
      handleServerData();
    });
  connect(mServerProcess,
          SELECT<QProcess::ProcessError>::OVERLOAD_OF(&QProcess::error),
          [this](QProcess::ProcessError error) {
      qDebug() << "SERVER ERROR" << error;
    });
  connect(mServerProcess, SELECT<int>::OVERLOAD_OF(&QProcess::finished), [this] {
      qDebug() << "SERVER GONE";
    });
  mServerProcess->setWorkingDirectory(buildDir);
  mServerProcess->start(cmakeExe + " -E daemon " + buildDir, QProcess::ReadWrite);
}

void CMakeClient::makeRequest(QJsonObject const& obj)
{
  QJsonDocument body(obj);
  QByteArray request;
  request += MAGIC_START;
  request += body.toJson();
  request += MAGIC_END;
  mServerProcess->write(request);
  Q_EMIT stdinWritten(request);
}

void CMakeClient::writeHandshake()
{
  QJsonObject obj;
  obj["type"] = "handshake";
  makeRequest(obj);
}

void CMakeClient::retrieveTargets()
{
  QJsonObject obj;
  obj["type"] = "buildsystem";

  makeRequest(obj);
}

void CMakeClient::retrieveContent(long line, const QString& filePath,
                                  const QString& fileContent)
{
  QJsonObject obj;
  obj["type"] = "content";
  obj["file_path"] = filePath;
  obj["file_line"] = (int)line;
  obj["file_content"] = fileContent;

  makeRequest(obj);
}

void CMakeClient::retrieveDiffContent(long line1, const QString& filePath1,
                                      long line2, const QString& filePath2,
                                      const QString& fileContent)
{
  QJsonObject obj;
  obj["type"] = "content_diff";
  obj["file_path1"] = filePath1;
  obj["file_line1"] = (int)line1;
  obj["file_content1"] = fileContent;
  obj["file_path2"] = filePath2;
  obj["file_line2"] = (int)line2;
  obj["file_content2"] = fileContent;

  makeRequest(obj);
}

void CMakeClient::retrieveParsed(const QString& filePath,
                                 const QString& content)
{
  QJsonObject obj;
  obj["type"] = "parse";
  obj["file_path"] = filePath;
  if (!content.isEmpty())
    {
    obj["file_content"] = content;
    }

  makeRequest(obj);
}

void CMakeClient::retrieveContextualHelp(const QString& filePath,
                                         int line, int column,
                                         const QString& fileContent)
{
  QJsonObject obj;
  obj["type"] = "contextual_help";
  obj["file_path"] = filePath;
  obj["line"] = line;
  obj["column"] = column;
  obj["file_content"] = fileContent;

  makeRequest(obj);
}

void CMakeClient::retrieveSources(const QString& targetName)
{
  QJsonObject obj;
  obj["type"] = "target_info";
  obj["target_name"] = targetName;
  obj["config"] = QString();

  makeRequest(obj);
}

void CMakeClient::retrieveCompletions(long line, long column,
                                      const QString& filePath,
                                      const QString& fileContent)
{
  QJsonObject obj;
  obj["type"] = "code_complete";
  obj["file_path"] = filePath;
  obj["file_line"] = (int)line;
  obj["file_column"] = (int)column;
  obj["file_content"] = fileContent;

  makeRequest(obj);
}

CMakeTarget::TargetType CMakeTarget::typeFromString(const QString& ts)
{
  if (ts == "EXECUTABLE")
    return EXECUTABLE;
  if (ts == "STATIC_LIBRARY")
    return STATIC_LIBRARY;
  if (ts == "SHARED_LIBRARY")
    return SHARED_LIBRARY;
  if (ts == "MODULE_LIBRARY")
    return MODULE_LIBRARY;
  if (ts == "OBJECT_LIBRARY")
    return OBJECT_LIBRARY;
  if (ts == "UTILITY")
    return UTILITY;
  if (ts == "GLOBAL_TARGET")
    return GLOBAL_TARGET;
  if (ts == "INTERFACE_LIBRARY")
    return INTERFACE_LIBRARY;
  return UTILITY;
}

QString CMakeTarget::stringFromType(CMakeTarget::TargetType t)
{
  if (t == EXECUTABLE)
    return "EXECUTABLE";
  if (t == STATIC_LIBRARY)
    return "STATIC_LIBRARY";
  if (t == SHARED_LIBRARY)
    return "SHARED_LIBRARY";
  if (t == MODULE_LIBRARY)
    return "MODULE_LIBRARY";
  if (t == OBJECT_LIBRARY)
    return "OBJECT_LIBRARY";
  if (t == UTILITY)
    return "UTILITY";
  if (t == GLOBAL_TARGET)
    return "GLOBAL_TARGET";
  if (t == INTERFACE_LIBRARY)
    return "INTERFACE_LIBRARY";
  return QString();
}
