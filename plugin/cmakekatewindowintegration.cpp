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

#include "cmakekatewindowintegration.h"

#include "cmakeclient.h"
#include "projectmodel.h"
#include "debugwidget.h"

#include <ktexteditor/plugin.h>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/application.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QAction>
#include <QTreeView>
#include <QStringListModel>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kcombobox.h>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <qmenu.h>
#include <qstring.h>
#include <QVBoxLayout>
#include <QApplication>
#include <QFileDialog>

CMakeKateWindowIntegration::CMakeKateWindowIntegration(KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mw)
: QObject (mw)
, KXMLGUIClient()
, m_mainWindow(mw)
, m_plugin(plugin)
{
  KXMLGUIClient::setComponentName (QLatin1String("cmakekate"), i18n ("CMake Kate Plugin"));
  setXMLFile(QStringLiteral("ui.rc"));

  auto a = actionCollection()->addAction(QStringLiteral("cmake_open_build"), this, SLOT(openBuildDialog()));
  a->setText(i18n("Open CMake Build"));

  mClient = new CMakeClient(this);

  m_mainWindow->guiFactory()->addClient(this);
}

void CMakeKateWindowIntegration::openBuild(QString const& buildDir)
{
  m_projectToolView = m_mainWindow->createToolView(m_plugin,
        QLatin1String ("kate_private_plugin_cmake_project"),
        KTextEditor::MainWindow::Left,
        QIcon::fromTheme (QLatin1String ("view-form-table")),
        i18nc("@title:window", "CMake Project")
  );

  m_stateBrowserToolView = m_mainWindow->createToolView(m_plugin,
        QLatin1String ("kate_private_plugin_cmake_state"),
        KTextEditor::MainWindow::Right,
        QIcon::fromTheme (QLatin1String ("view-list-tree")),
        i18nc("@title:window", "CMake State")
  );

//   connect(mClient, &CMakeClient::stdoutReceieved, this,
//           [this](const QString& newBit) {
//       qDebug() << "INCOMING" << newBit;
//     });
//
//   connect(mClient, &CMakeClient::stdinWritten, this,
//           [this](const QString& newBit) {
//       qDebug() << "OUTGOING" << newBit;
//     });

  mClient->start("/home/stephen/dev/prefix/qt/kde/bin/cmake", buildDir);

  m_mainWindow->showToolView(m_projectToolView);
  m_mainWindow->showToolView(m_stateBrowserToolView);

  auto projectTree = new QTreeView(m_projectToolView);
  mProjectModel = new ProjectModel(mClient, this);
  projectTree->setModel(mProjectModel);

  connect(mProjectModel, &QAbstractItemModel::modelReset, this, [this, projectTree]{
      projectTree->expandAll();
    });

  connect(projectTree->selectionModel(), &QItemSelectionModel::selectionChanged,
          [this] (const QItemSelection& selected){
    auto idxs = selected.indexes();
    if (idxs.size() == 1) {
      auto path = QUrl::fromLocalFile(idxs[0].data(ProjectModel::FullPath).toString());
      auto ktev = m_mainWindow->openUrl(path);
      mDebugWidget->setView(ktev);
    }
  });

  mDebugWidget = new DebugWidget(mClient, m_stateBrowserToolView);
}

void CMakeKateWindowIntegration::openBuildDialog()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QStringList urls = dialog.selectedFiles();
    if (urls.size() != 1)
    {
      return;
    }

    openBuild(urls[0]);
}

CMakeKateWindowIntegration::~CMakeKateWindowIntegration()
{
}
