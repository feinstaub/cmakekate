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

#include "projectmodel.h"
#include "cmakeclient.h"

#include <QDir>
#include <QPixmap>
#include <QDebug>

ProjectModel::ProjectModel(CMakeClient* client, QObject *parent)
  : QAbstractItemModel(parent), mClient(client)
{
  auto requestTargets = [this](){
      if (m_data.childItems.isEmpty()
          && mClient->GetState() == CMakeClient::Idle)
        {
          mClient->retrieveTargets();
        }
    };
  connect(mClient, &CMakeClient::stateChanged, this, requestTargets);
  auto handleTargets = [this](QStringList const& configs,
      QVector<CMakeTarget> const& targets){
      beginResetModel();
      setDataFromTargets(targets);
      endResetModel();
    };
  connect(mClient, &CMakeClient::targetsRetrieved, this, handleTargets);
  requestTargets();

  connect(mClient, &CMakeClient::sourcesRetrieved, this,
          [this](QString tgt, QStringList srcs) {
      beginResetModel();
      addSourcesToTarget(tgt, srcs);
      endResetModel();
  });
}

std::vector<CMakeTarget> ProjectModel::GetTargets() const
{
  std::vector<CMakeTarget> ret;

  for (auto& tgt: m_data.targets)
    {
      ret.push_back(tgt);
    }

  return ret;
}

quintptr ProjectModel::parentId(const QString& path_)
{
  QFileInfo fi(path_);
  QString path = QFileInfo(fi.canonicalPath()).canonicalPath();
  auto srcDir = QFileInfo(m_data.srcLocation).path();
  while (!QFileInfo(path + "/CMakeLists.txt").exists())
  {
    path = QFileInfo(path).canonicalPath();
  }
  while (path != srcDir) {
    for(auto it = m_data.locations.begin(); it != m_data.locations.end(); ++it) {
      QFileInfo fit(it.value());
      if (fit.canonicalPath() == path) {
        return it.key();
      }
    }
    if (path != srcDir) {
      auto newId = m_nextId++;
      auto loc = path + "/CMakeLists.txt";
      auto pid = parentId(loc);
      m_data.childItems[pid].append(newId);
      m_data.locations[newId] = loc;
      return newId;
    }
    QFileInfo fi(path);
    path = fi.absolutePath();
  }
  return 1;
}

void ProjectModel::addSourcesToTarget(quintptr id, QStringList srcs)
{
  for (auto src: srcs)
  {
    auto srcId = m_nextId++;
    m_data.childItems[id].append(srcId);
    m_data.Sources[srcId] = src;
    m_data.locations[srcId] = src;
  }
}

void ProjectModel::addSourcesToTarget(const QString& tgtName, QStringList srcs)
{
  qDebug() << tgtName;
  for (auto it = m_data.childItems.begin(); it != m_data.childItems.end(); ++it)
  {
    for (auto i = it.value().begin(); i < it.value().end(); ++i)
      {
      if (m_data.targets.contains(*i))
        {
        if (m_data.targets[*i].Name == tgtName)
          {
            addSourcesToTarget(it.key(), srcs);
            break;
          }
        }
      }
  }
}

void ProjectModel::setDataFromTargets(const QVector<CMakeTarget>& targets)
{
  m_data = ProjectData();
  QString srcDir = mClient->sourceDir();
  m_data.srcLocation = QDir::cleanPath(srcDir + "/CMakeLists.txt");

  m_data.projectName = mClient->projectName();

  m_nextId = 1;

  m_data.locations[m_nextId++] = m_data.srcLocation;

  foreach(auto& target, targets) {
    if (target.Type != CMakeTarget::UTILITY) {
      QString location;
      int btIndex = 0;
      for ( ; btIndex < target.Backtrace.size(); ++btIndex) {
        QString btPath = srcDir + "/" + target.Backtrace[btIndex].first;
        if (btPath.endsWith("/CMakeLists.txt")) {
          auto ifo = QFileInfo(btPath);
          location = ifo.canonicalFilePath();
          break;
        }
      }
      if (location.isEmpty())
        continue;

      QFileInfo fi(location);
      auto dir = fi.canonicalPath();

      quintptr pid = 1;
      bool found = false;
      for(auto it = m_data.locations.begin(); it != m_data.locations.end(); ++it) {
        QFileInfo fit(it.value());
        if (fit.canonicalPath() == dir) {
          pid = it.key();
          found = true;
          break;
        }
      }
      if (!found) {
        pid = m_nextId++;
        auto lpid = parentId(location);
        m_data.childItems[lpid].append(pid);
        m_data.locations[pid] = location;
      }

      auto tgtId = m_nextId++;
      m_data.childItems[pid].append(tgtId);
      m_data.targets[tgtId].Name = target.Name;
      m_data.targets[tgtId].Type = target.Type;

      m_data.targets[tgtId].Path = location;
      m_data.targets[tgtId].Line = target.Backtrace[btIndex].second - 1;

      mClient->retrieveSources(target.Name);
    }
  }
  m_data.childItems[0].push_back(1);
}

int ProjectModel::rowCount(const QModelIndex& parent) const
{
  if (m_data.childItems.isEmpty())
    {
      return 0;
    }
  quintptr grandParentId = parent.internalId();
  Q_ASSERT(m_data.childItems.contains(grandParentId));

  if (!parent.isValid())
    {
      return m_data.childItems[0].size();
    }
  Q_ASSERT(m_data.childItems[grandParentId].size() > parent.row());

  auto parentId = m_data.childItems[grandParentId][parent.row()];

  if (!m_data.childItems.contains(parentId)) {
    return 0;
  }

  return m_data.childItems[parentId].size();
}

int ProjectModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex& parent) const
{
  if (row < 0 || column < 0 || !hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid())
    return createIndex(row, column);

  quintptr grandParentId = parent.internalId();

  Q_ASSERT(m_data.childItems.contains(grandParentId));
  Q_ASSERT(m_data.childItems[grandParentId].size() > parent.row());
  quintptr parentId = m_data.childItems[grandParentId][parent.row()];

  Q_ASSERT(m_data.childItems[parentId].size() > row);

  return createIndex(row, column, parentId);
}

QModelIndex ProjectModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();
  quintptr id = index.internalId();
  if (id == 0)
    {
      return QModelIndex();
    }

  for(auto it = m_data.childItems.begin(); it != m_data.childItems.end(); ++it) {
    auto position = it.value().indexOf(id);
    if (position != -1) {
        return createIndex(position, 0, it.key());
    }
  }
  return QModelIndex();
}

QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();
  if (role == Qt::DisplayRole || role == Qt::DecorationRole
      || (role >= FullPath && role < LastCustomRole)) {
    auto parentId = index.internalId();
    Q_ASSERT(m_data.childItems.contains(parentId));
    Q_ASSERT(m_data.childItems[parentId].size() > index.row());
    auto id = m_data.childItems[parentId][index.row()];
    if (m_data.targets.contains(id)) {
      if (role == Qt::DisplayRole || role == TargetName) {
        return m_data.targets[id].Name;
      } else if (role == FullPath) {
        return m_data.targets[id].Path;
      } else if (role == Qt::DecorationRole) {
        return QPixmap(":/qt-project.org/styles/commonstyle/images/standardbutton-yes-16.png");
      } else {
        return m_data.targets[id].Line;
      }
    } else if (m_data.Sources.contains(id)) {
      if (role == Qt::DisplayRole) {
        auto fi = QFileInfo(m_data.Sources[id]);
        return fi.fileName();
      } else if (role == FullPath) {
        return m_data.Sources[id];
      }
    } else {
      Q_ASSERT(m_data.locations.contains(id));

      QFileInfo fi(m_data.locations[id]);
      if (role == Qt::DisplayRole) {
        if (!index.parent().isValid()) {
          return m_data.projectName;
        }
        auto parentDir = QFileInfo(index.parent().data(FullPath).toString()).dir();
        return parentDir.relativeFilePath(fi.canonicalPath());
      } else if (role == Qt::DecorationRole) {
        if (!index.parent().isValid()) {
          return QPixmap(":/qt-project.org/styles/commonstyle/images/viewlist-16.png");
        }
        return QPixmap(":/qt-project.org/styles/commonstyle/images/diropen-16.png");
      } else {
        return fi.canonicalFilePath();
      }
    }
  }
  return QVariant();
}

bool operator==(const ProjectData& lhs, const ProjectData& rhs)
{
  return lhs.targets == rhs.targets
      && lhs.locations == rhs.locations
      && lhs.childItems == rhs.childItems
      && lhs.buildDir == rhs.buildDir
      && lhs.srcLocation == rhs.srcLocation
      && lhs.cmakeExe == rhs.cmakeExe
      && lhs.projectName == rhs.projectName;
}
