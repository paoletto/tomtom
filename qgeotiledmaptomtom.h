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

#ifndef QGEOTILEDMAPTOMTOM_H
#define QGEOTILEDMAPTOMTOM_H

#include <QtLocation/private/qgeotiledmap_p.h>

#ifdef LOCATIONLABS
#include <QtLocation/private/qgeotiledmaplabs_p.h>
typedef QGeoTiledMapLabs Map;
#else
typedef QGeoTiledMap Map;
#endif

QT_BEGIN_NAMESPACE

class QGeoTiledMappingManagerEngineTomTom;
class QGeoTiledMapTomTom: public Map
{
    Q_OBJECT

public:
    QGeoTiledMapTomTom(QGeoTiledMappingManagerEngineTomTom *engine, QObject *parent = nullptr);
    ~QGeoTiledMapTomTom() override;

protected:
    void evaluateCopyrights(const QSet<QGeoTileSpec> &visibleTiles) override;

private:
    QString m_copyrights;
    QGeoTiledMappingManagerEngineTomTom *m_engine;
};

QT_END_NAMESPACE

#endif
