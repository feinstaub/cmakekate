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

#include <QAbstractItemModel>

#include "utility.h"

class CMakeClient;
class CMakeTarget;

struct Target
{
#define DECLARE_MEMBER(NAME) QStringList NAME;
  CMB_FOR_EACH_SOURCE_TYPE(DECLARE_MEMBER)
  CMB_FOR_EACH_BUILD_PROPERTY_TYPE(DECLARE_MEMBER)
#undef DECLARE_MEMBER

  QString name;
  QString path;
  QString type;
  QString file;

  QStringList backtraceFiles;
  QList<uint> backtraceLines;

  uint line;
};

inline bool operator==(const Target& lhs, const Target& rhs)
{
  return
#define COMPARE_MEMBER(NAME) lhs.NAME == rhs.NAME &&
  CMB_FOR_EACH_SOURCE_TYPE(COMPARE_MEMBER)
  CMB_FOR_EACH_BUILD_PROPERTY_TYPE(COMPARE_MEMBER)
#undef COMPARE_MEMBER
       lhs.name == rhs.name
    && lhs.path == rhs.path
    && lhs.type == rhs.type
    && lhs.file == rhs.file
    && lhs.line == rhs.line
    && lhs.backtraceFiles == rhs.backtraceFiles
    && lhs.backtraceLines == rhs.backtraceLines;
}

inline bool operator!=(const Target& lhs, const Target& rhs)
{
  return !(lhs == rhs);
}

QDebug operator<<(QDebug, const Target&);

class ProjectData
{
public:
  QHash<quintptr, CMakeTarget> targets;
  QHash<quintptr, QString> locations;
  QHash<quintptr, QVector<quintptr> > childItems;
  QHash<quintptr, QString> Sources;
  QString buildDir;
  QString srcLocation;
  QString cmakeExe;
  QString projectName;
};

bool operator==(const ProjectData& lhs, const ProjectData& rhs);

inline bool operator!=(const ProjectData& lhs, const ProjectData& rhs)
{
  return !(lhs == rhs);
}

class ProjectModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ProjectModel(CMakeClient* client, QObject *parent = 0);

  enum Data {
    FullPath = Qt::UserRole + 1,
    Line,
    TargetName,
    LastCustomRole
  };

  std::vector<CMakeTarget> GetTargets() const;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& parent) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
  void setDataFromTargets(const QVector<CMakeTarget>& targets);
  quintptr parentId(const QString& path_);

  void addSourcesToTarget(quintptr id, QStringList srcs);
  void addSourcesToTarget(const QString& tgtName, QStringList srcs);

private:
  ProjectData m_data;
  CMakeClient* mClient;
  QStringList mConfigs;
  long m_nextId = 1;
};
