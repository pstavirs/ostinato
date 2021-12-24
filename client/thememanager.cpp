/*
Copyright (C) 2021 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "thememanager.h"

#include "settings.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QResource>

ThemeManager *ThemeManager::instance_{nullptr};

ThemeManager::ThemeManager()
{
#ifdef Q_OS_WIN32
    themeDir_ = QCoreApplication::applicationDirPath() + "/themes/";
#elif Q_OS_MAC
    // TODO
#else
    // TODO
#endif
}

QStringList ThemeManager::themeList()
{
    QDir themeDir(themeDir_);

    themeDir.setFilter(QDir::Files);
    themeDir.setNameFilters(QStringList() << "*.qss");
    themeDir.setSorting(QDir::Name);

    QStringList themes = themeDir.entryList();
    for (QString& theme : themes)
        theme.remove(".qss");
    themes.prepend("default");

    return themes;
}

void ThemeManager::setTheme(QString theme)
{
    // Remove current theme, if we have one
    QString oldTheme = appSettings->value(kThemeKey).toString();
    if (!oldTheme.isEmpty()) {
        // Remove stylesheet first so that there are
        // no references to resources when unregistering 'em
        qApp->setStyleSheet("");
        QString rccFile = themeDir_ + oldTheme + ".rcc";
        if (QResource::unregisterResource(rccFile)) {
            qDebug("Unable to unregister theme rccFile %s",
                   qPrintable(rccFile));
        }
        appSettings->setValue(kThemeKey, QVariant());
    }

    if (theme.isEmpty() || (theme == "default"))
        return;

    // Apply new theme
    QFile qssFile(themeDir_ + theme + ".qss");
    if (!qssFile.open(QFile::ReadOnly)) {
        qDebug("Unable to open theme qssFile %s",
               qPrintable(qssFile.fileName()));
        return;
    }

    // Register theme resource before applying theme style sheet
    QString rccFile = themeDir_ + theme + ".rcc";
    if (!QResource::registerResource(rccFile))
        qDebug("Unable to register theme rccFile %s", qPrintable(rccFile));

#if 0 // FIXME: debug only
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }
#endif

    QString styleSheet { qssFile.readAll() };
    qApp->setStyleSheet(styleSheet);
    appSettings->setValue(kThemeKey, theme);
}

ThemeManager* ThemeManager::instance()
{
    if (!instance_)
        instance_ = new ThemeManager();
    return instance_;
}
