/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "property.h"
#include "option.h"

#include <qdir.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <stdio.h>

QT_BEGIN_NAMESPACE

static const struct {
    const char *name;
    QLibraryInfo::LibraryLocation loc;
    bool raw;
} propList[] = {
    { "QT_SYSROOT", QLibraryInfo::SysrootPath, true },
    { "QT_INSTALL_PREFIX", QLibraryInfo::PrefixPath, false },
    { "QT_INSTALL_DATA", QLibraryInfo::DataPath, false },
    { "QT_INSTALL_DOCS", QLibraryInfo::DocumentationPath, false },
    { "QT_INSTALL_HEADERS", QLibraryInfo::HeadersPath, false },
    { "QT_INSTALL_LIBS", QLibraryInfo::LibrariesPath, false },
    { "QT_INSTALL_BINS", QLibraryInfo::BinariesPath, false },
    { "QT_INSTALL_TESTS", QLibraryInfo::TestsPath, false },
    { "QT_INSTALL_PLUGINS", QLibraryInfo::PluginsPath, false },
    { "QT_INSTALL_IMPORTS", QLibraryInfo::ImportsPath, false },
    { "QT_INSTALL_TRANSLATIONS", QLibraryInfo::TranslationsPath, false },
    { "QT_INSTALL_CONFIGURATION", QLibraryInfo::SettingsPath, false },
    { "QT_INSTALL_EXAMPLES", QLibraryInfo::ExamplesPath, false },
    { "QT_INSTALL_DEMOS", QLibraryInfo::ExamplesPath, false }, // Just backwards compat
    { "QT_RAW_INSTALL_PREFIX", QLibraryInfo::PrefixPath, true },
    { "QT_RAW_INSTALL_DATA", QLibraryInfo::DataPath, true },
    { "QT_RAW_INSTALL_DOCS", QLibraryInfo::DocumentationPath, true },
    { "QT_RAW_INSTALL_HEADERS", QLibraryInfo::HeadersPath, true },
    { "QT_RAW_INSTALL_LIBS", QLibraryInfo::LibrariesPath, true },
    { "QT_RAW_INSTALL_BINS", QLibraryInfo::BinariesPath, true },
    { "QT_RAW_INSTALL_TESTS", QLibraryInfo::TestsPath, true },
    { "QT_RAW_INSTALL_PLUGINS", QLibraryInfo::PluginsPath, true },
    { "QT_RAW_INSTALL_IMPORTS", QLibraryInfo::ImportsPath, true },
    { "QT_RAW_INSTALL_TRANSLATIONS", QLibraryInfo::TranslationsPath, true },
    { "QT_RAW_INSTALL_CONFIGURATION", QLibraryInfo::SettingsPath, true },
    { "QT_RAW_INSTALL_EXAMPLES", QLibraryInfo::ExamplesPath, true },
    { "QT_HOST_PREFIX", QLibraryInfo::HostPrefixPath, true },
    { "QT_HOST_DATA", QLibraryInfo::HostDataPath, true },
    { "QT_HOST_BINS", QLibraryInfo::HostBinariesPath, true },
};

QMakeProperty::QMakeProperty() : settings(0)
{
    for (int i = 0; i < sizeof(propList)/sizeof(propList[0]); i++)
        m_values[QString::fromLatin1(propList[i].name)] = propList[i].raw
                ? QLibraryInfo::rawLocation(propList[i].loc)
                : QLibraryInfo::location(propList[i].loc);
}

QMakeProperty::~QMakeProperty()
{
    delete settings;
    settings = 0;
}

void QMakeProperty::initSettings()
{
    if(!settings) {
        settings = new QSettings(QSettings::UserScope, "Trolltech", "QMake");
        settings->setFallbacksEnabled(false);
    }
}

QString
QMakeProperty::keyBase(bool version) const
{
    if (version)
        return QString(qmake_version()) + "/";
    return QString();
}

QString
QMakeProperty::value(QString v, bool just_check)
{
    QString val = m_values.value(v);
    if (!val.isNull())
        return val;
    else if(v == "QMAKE_MKSPECS")
        return Option::mkspecPaths().join(Option::dirlist_sep);
    else if(v == "QMAKE_VERSION")
        return qmake_version();
#ifdef QT_VERSION_STR
    else if(v == "QT_VERSION")
        return QT_VERSION_STR;
#endif

    initSettings();
    int slash = v.lastIndexOf('/');
    QVariant var = settings->value(keyBase(slash == -1) + v);
    bool ok = var.isValid();
    QString ret = var.toString();
    if(!ok) {
        QString version = qmake_version();
        if(slash != -1) {
            version = v.left(slash-1);
            v = v.mid(slash+1);
        }
        settings->beginGroup(keyBase(false));
        QStringList subs = settings->childGroups();
        settings->endGroup();
        subs.sort();
        for (int x = subs.count() - 1; x >= 0; x--) {
            QString s = subs[x];
            if(s.isEmpty() || s > version)
                continue;
            var = settings->value(keyBase(false) + s + "/" + v);
            ok = var.isValid();
            ret = var.toString();
            if (ok) {
                if(!just_check)
                    debug_msg(1, "Fell back from %s -> %s for '%s'.", version.toLatin1().constData(),
                              s.toLatin1().constData(), v.toLatin1().constData());
                return ret;
            }
        }
    }
    return ok ? ret : QString();
}

bool
QMakeProperty::hasValue(QString v)
{
    return !value(v, true).isNull();
}

void
QMakeProperty::setValue(QString var, const QString &val)
{
    initSettings();
    settings->setValue(keyBase() + var, val);
}

void
QMakeProperty::remove(const QString &var)
{
    initSettings();
    settings->remove(keyBase() + var);
}

bool
QMakeProperty::exec()
{
    bool ret = true;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY) {
        if(Option::prop::properties.isEmpty()) {
            initSettings();
            settings->beginGroup(keyBase(false));
            QStringList subs = settings->childGroups();
            settings->endGroup();
            subs.sort();
            for(int x = subs.count() - 1; x >= 0; x--) {
                QString s = subs[x];
                if(s.isEmpty())
                    continue;
                settings->beginGroup(keyBase(false) + s);
                QStringList keys = settings->childKeys();
                settings->endGroup();
                for(QStringList::ConstIterator it2 = keys.begin(); it2 != keys.end(); it2++) {
                    QString ret = settings->value(keyBase(false) + s + "/" + (*it2)).toString();
                    if(s != qmake_version())
                        fprintf(stdout, "%s/", s.toLatin1().constData());
                    fprintf(stdout, "%s:%s\n", (*it2).toLatin1().constData(), ret.toLatin1().constData());
                }
            }
            QStringList specialProps;
            for (int i = 0; i < sizeof(propList)/sizeof(propList[0]); i++)
                specialProps.append(QString::fromLatin1(propList[i].name));
            specialProps.append("QMAKE_MKSPECS");
            specialProps.append("QMAKE_VERSION");
#ifdef QT_VERSION_STR
            specialProps.append("QT_VERSION");
#endif
            foreach (QString prop, specialProps)
                fprintf(stdout, "%s:%s\n", prop.toLatin1().constData(), value(prop).toLatin1().constData());
            return true;
        }
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            if(Option::prop::properties.count() > 1)
                fprintf(stdout, "%s:", (*it).toLatin1().constData());
            if(!hasValue((*it))) {
                ret = false;
                fprintf(stdout, "**Unknown**\n");
            } else {
                fprintf(stdout, "%s\n", value((*it)).toLatin1().constData());
            }
        }
    } else if(Option::qmake_mode == Option::QMAKE_SET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            it++;
            if(it == Option::prop::properties.end()) {
                ret = false;
                break;
            }
            if(!var.startsWith("."))
                setValue(var, (*it));
        }
    } else if(Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY) {
        for(QStringList::ConstIterator it = Option::prop::properties.begin();
            it != Option::prop::properties.end(); it++) {
            QString var = (*it);
            if(!var.startsWith("."))
                remove(var);
        }
    }
    return ret;
}

QT_END_NAMESPACE
