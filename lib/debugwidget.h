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

#include <QWidget>

#include "projectmodel.h"

#include <ktexteditor/view.h>

class QProcess;
class QPushButton;
class CMakeClient;
class QStandardItemModel;
class QTreeView;
class QLineEdit;
class QSortFilterProxyModel;

class DebugWidget : public QWidget
{
  Q_OBJECT
public:
  DebugWidget(CMakeClient* client, QWidget* parent = 0);

  void setView(KTextEditor::View* ktev);

private Q_SLOTS:
  void getDebugInfo();

  void setContent(QMap<QString, QString> const& defs);

  void setDiffContent(QMap<QString, QString> const& newDefs,
                   QMap<QString, QString> const& oldDefs);

  void updateCursorPos();

Q_SIGNALS:
  void cursorBlocksChanged();

private:
  KTextEditor::View* mKtev;
  QStandardItemModel* m_defsModel;
  QTreeView* m_defsView;
  QLineEdit *m_filterLineEdit;
  QSortFilterProxyModel *m_filter;
  CMakeClient* mClient;
  int mPosLine;
  int mAnchorLine;
};
