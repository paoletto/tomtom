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

#include "qgeofiletilecachetomtom.h"
#include <QtLocation/private/qgeotilespec_p.h>
#include <QDir>

QT_BEGIN_NAMESPACE

QGeoFileTileCacheTomTom::QGeoFileTileCacheTomTom(const QList<QGeoMapType> &/*mapTypes*/, int scaleFactor, const QString &directory, QObject *parent)
    :QGeoFileTileCache(directory, parent)
{
    m_scaleFactor = qBound(1, scaleFactor, 2);
}

QGeoFileTileCacheTomTom::~QGeoFileTileCacheTomTom()
{

}

QString QGeoFileTileCacheTomTom::tileSpecToFilename(const QGeoTileSpec &spec, const QString &format, const QString &directory) const
{
    QString filename = spec.plugin();
    filename += QLatin1String("-");
    filename += QString::number(spec.mapId());
    filename += QLatin1String("-");
    filename += QString::number(spec.zoom());
    filename += QLatin1String("-");
    filename += QString::number(spec.x());
    filename += QLatin1String("-");
    filename += QString::number(spec.y());

    //Append version if real version number to ensure backwards compatibility and eviction of old tiles
    if (spec.version() != -1) {
        filename += QLatin1String("-");
        filename += QString::number(spec.version());
    }

    filename += QLatin1String("-@");
    filename += QString::number(m_scaleFactor);
    filename += QLatin1Char('x');

    filename += QLatin1String(".");
    filename += format;

    QDir dir = QDir(directory);
    const QString res = dir.filePath(filename);
    return res;
}

QGeoTileSpec QGeoFileTileCacheTomTom::filenameToTileSpec(const QString &filename) const
{
    QStringList parts = filename.split('.');

    if (Q_UNLIKELY(parts.length() != 2)) {
        qWarning() << "QGeoFileTileCacheTomTom::filenameToTileSpec More than 1 dot in the filename!";
        return QGeoTileSpec();
    }

    QString name = parts.at(0) + QChar('.') + parts.at(1);
    QStringList fields = name.split('-');

    int length = fields.length();
    if (Q_UNLIKELY(length != 6 && length != 7)) {
        qWarning() << "QGeoFileTileCacheTomTom::filenameToTileSpec fields aren't 6 or 7!";
        return QGeoTileSpec();
    } else {
        int scaleIdx = fields.last().indexOf("@");
        if (Q_UNLIKELY(scaleIdx < 0 || fields.last().size() <= (scaleIdx + 2))) {
            qWarning() << "QGeoFileTileCacheTomTom::filenameToTileSpec wrong scaleIdx!";
            return QGeoTileSpec();
        }
        int scaleFactor = fields.last()[scaleIdx + 1].digitValue();
        if (scaleFactor != m_scaleFactor)
           return QGeoTileSpec();
    }

    QList<int> numbers;

    bool ok = false;
    for (int i = 2; i < length-1; ++i) { // skipping -@_X
        ok = false;
        int value = fields.at(i).toInt(&ok);
        if (Q_UNLIKELY(!ok)) {
            qWarning() << "QGeoFileTileCacheTomTom::filenameToTileSpec failed to parse int!" << fields.at(i);
            return QGeoTileSpec();
        }
        numbers.append(value);
    }

    //File name without version, append default
    if (numbers.length() < 4)
        numbers.append(-1);

    const int mapId = fields.at(1).toInt(&ok);
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "QGeoFileTileCacheTomTom::filenameToTileSpec failed to parse mapId!" << fields.at(1);
        return QGeoTileSpec();
    }

    QGeoTileSpec res(fields.at(0),
                    mapId,
                    numbers.at(0),
                    numbers.at(1),
                    numbers.at(2),
                    numbers.at(3));
    return res;
}

QT_END_NAMESPACE
