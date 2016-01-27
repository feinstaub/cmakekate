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

#ifndef CMAKEKATEPLUGIN_H
#define CMAKEKATEPLUGIN_H

#include <QUrl>
#include <QVariant>

#include <KTextEditor/Plugin>

class CMakeKatePlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    public:
        explicit CMakeKatePlugin(QObject *parent = 0, const QList<QVariant> & = QList<QVariant>());
        virtual ~CMakeKatePlugin();

        QObject *createView(KTextEditor::MainWindow *mainWindow);
};

#endif
