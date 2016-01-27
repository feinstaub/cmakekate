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

#include "cmakekateplugin.h"
#include "cmakekatewindowintegration.h"

#include <KConfigGroup>
#include <KDirWatch>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDir>
#include <QDebug>

K_PLUGIN_FACTORY_WITH_JSON(CMakeKatePluginFactory,
                           "cmakekateplugin.json",
                           registerPlugin<CMakeKatePlugin>();)

CMakeKatePlugin::CMakeKatePlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
}

CMakeKatePlugin::~CMakeKatePlugin()
{
}

QObject *CMakeKatePlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    return new CMakeKateWindowIntegration(this, mainWindow);
}

#include "cmakekateplugin.moc"
