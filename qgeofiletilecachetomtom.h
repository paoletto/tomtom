/****************************************************************************
**
** Copyright (C) 2024- Paolo Angelelli <paoletto@gmail.com>
**
** Commercial License Usage
** Licensees holding a valid commercial qdemviewer license may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement with the copyright holder. For licensing terms
** and conditions and further information contact the copyright holder.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3. The licenses are as published by
** the Free Software Foundation at https://www.gnu.org/licenses/gpl-3.0.html,
** with the exception that the use of this work for training artificial intelligence
** is prohibited for both commercial and non-commercial use.
**
****************************************************************************/

#ifndef QGEOFILETILECACHETOMTOM_H
#define QGEOFILETILECACHETOMTOM_H

#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QMap>

QT_BEGIN_NAMESPACE

class QGeoFileTileCacheTomTom : public QGeoFileTileCache
{
    Q_OBJECT
public:
    QGeoFileTileCacheTomTom(const QList<QGeoMapType> &mapTypes, int scaleFactor, const QString &directory = QString(), QObject *parent = 0);
    ~QGeoFileTileCacheTomTom();

protected:
    QString tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, const QString &directory) const override;
    QGeoTileSpec filenameToTileSpec(const QString &filename) const override;

    int m_scaleFactor;
};

QT_END_NAMESPACE

#endif // QGEOFILETILECACHETOMTOM_H
