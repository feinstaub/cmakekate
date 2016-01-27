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

#include "debugwidget.h"

#include "cmakeclient.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QSortFilterProxyModel>

DebugWidget::DebugWidget(CMakeClient* client, QWidget* parent)
  : QWidget(parent), mKtev(0), mClient(client)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  connect(mClient, &CMakeClient::contentRetrieved,
          this, &DebugWidget::setContent);

  connect(mClient, &CMakeClient::diffContentRetrieved,
          this, &DebugWidget::setDiffContent);

  connect(this, &DebugWidget::cursorBlocksChanged,
          this, &DebugWidget::getDebugInfo);

  mPosLine = 1;
  mAnchorLine = 1;

  layout->addWidget(new QLabel("Definitions"));

  m_filterLineEdit = new QLineEdit;
  layout->addWidget(m_filterLineEdit);

  m_defsView = new QTreeView;
  m_defsView->setRootIsDecorated(false);
  m_defsView->setVerticalScrollMode(QTreeView::ScrollPerPixel);
  m_defsModel = new QStandardItemModel;

  m_filter = new QSortFilterProxyModel;
  m_filter->setSourceModel(m_defsModel);

  connect(m_filterLineEdit, &QLineEdit::textChanged, [this]{
      m_filter->setFilterRegExp(m_filterLineEdit->text());
    });

  m_defsView->setModel(m_filter);
  layout->addWidget(m_defsView);
}

void DebugWidget::setView(KTextEditor::View* ktev)
{
  mKtev = ktev;
  connect(mKtev, &KTextEditor::View::cursorPositionChanged, [this] {
    updateCursorPos();
  });
}

QString format(QString const& input)
{
  return input.split(";").join("\n");
}

void DebugWidget::setContent(QMap<QString, QString> const& defs)
{
  m_defsModel->clear();

  for(auto key : defs.keys())
    {
       m_defsModel->appendRow({new QStandardItem(key),
                              new QStandardItem(defs.value(key))});
       m_defsModel->item(m_defsModel->rowCount() - 1)
           ->setData(format(defs.value(key)), Qt::ToolTipRole);
       m_defsModel->item(m_defsModel->rowCount() - 1, 1)
           ->setData(format(defs.value(key)), Qt::ToolTipRole);
    }

  m_defsView->resizeColumnToContents(0);
}

void DebugWidget::setDiffContent(QMap<QString, QString> const& newDefs,
                              QMap<QString, QString> const& oldDefs)
{
  m_defsModel->clear();

  for(auto key : newDefs.keys())
    {
      m_defsModel->appendRow({new QStandardItem(key),
                              new QStandardItem(newDefs.value(key))});
      m_defsModel->item(m_defsModel->rowCount() - 1)
          ->setData(QPixmap(":/list-add.png"), Qt::DecorationRole);
      m_defsModel->item(m_defsModel->rowCount() - 1)
          ->setData(format(newDefs.value(key)), Qt::ToolTipRole);
      m_defsModel->item(m_defsModel->rowCount() - 1, 1)
          ->setData(format(newDefs.value(key)), Qt::ToolTipRole);
    }

  for(auto key : oldDefs.keys())
    {
      m_defsModel->appendRow({new QStandardItem(key),
                              new QStandardItem(oldDefs.value(key))});
      m_defsModel->item(m_defsModel->rowCount() - 1)
          ->setData(QPixmap(":/list-remove.png"), Qt::DecorationRole);
      m_defsModel->item(m_defsModel->rowCount() - 1)
          ->setData(format(oldDefs.value(key)), Qt::ToolTipRole);
      m_defsModel->item(m_defsModel->rowCount() - 1, 1)
          ->setData(format(oldDefs.value(key)), Qt::ToolTipRole);
    }

  m_defsView->resizeColumnToContents(0);
}

void DebugWidget::updateCursorPos()
{
  auto range = mKtev->selectionRange();
  int newAnchorLine = 0;
  int newPosLine = 0;
  if (range.isValid())
    {
    newAnchorLine = range.start().line() + 1;
    newPosLine = range.end().line() + 1;
    }
  else
    {
    auto cursor = mKtev->cursorPosition();
    newAnchorLine = cursor.line() + 1;
    newPosLine = cursor.line() + 1;
    }
  bool hadChange = false;
  if (mAnchorLine != newAnchorLine)
    {
    mAnchorLine = newAnchorLine;
    hadChange = true;
    }
  if (mPosLine != newPosLine)
    {
    mPosLine = newPosLine;
    hadChange = true;
    }
  if (hadChange)
    {
    Q_EMIT cursorBlocksChanged();
    }
}

void DebugWidget::getDebugInfo()
{
  if (mKtev->document()->url().toLocalFile().isEmpty())
  {
    return;
  }
  if (mPosLine != mAnchorLine)
    {
    mClient->retrieveDiffContent(mAnchorLine,
                                 mKtev->document()->url().toLocalFile(),
                                 mPosLine,
                                 mKtev->document()->url().toLocalFile(),
                                 mKtev->document()->text());
    }
  else
    {
    mClient->retrieveContent(mPosLine,
                             mKtev->document()->url().toLocalFile(),
                             mKtev->document()->text());
    }
}
