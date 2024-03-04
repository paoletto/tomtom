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

#include "qgeotiledmappingmanagerenginetomtom.h"
#include "qgeotiledmaptomtom.h"
#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#include <QtLocation/private/qgeotiledmap_p.h>
#include "qtomtomcommon.h"
#include "qgeotilefetchertomtom.h"
#include "qgeofiletilecachetomtom.h"
#include "qgeotiledmaptomtom.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>


QT_BEGIN_NAMESPACE


QGeoTiledMappingManagerEngineTomTom::QGeoTiledMappingManagerEngineTomTom(const QVariantMap &parameters,
                                                                         QGeoServiceProvider::Error *error,
                                                                         QString *errorString)
:   QGeoTiledMappingManagerEngine()
{
    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(0.0);
    cameraCaps.setMaximumZoomLevel(22.0);
    cameraCaps.setSupportsBearing(true);
    cameraCaps.setSupportsTilting(true);
    cameraCaps.setMinimumTilt(0);
    cameraCaps.setMaximumTilt(80);
    cameraCaps.setMinimumFieldOfView(20.0);
    cameraCaps.setMaximumFieldOfView(120.0);
    cameraCaps.setOverzoomEnabled(true);
    setCameraCapabilities(cameraCaps);

    setTileSize(QSize(256, 256));

    const QByteArray pluginName = "tomtom";
    QList<QGeoMapType> mapTypes;
    QByteArray userAgent = QTomTomCommon::userAgent;
    if (parameters.contains(QStringLiteral("tomtom.useragent")))
        userAgent += QLatin1String(" - ") + parameters.value(QStringLiteral("tomtom.useragent")).toString().toLatin1();

    const QString accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    setTileSize(QSize(256, 256));

    //: Noun describing map type 'Street map'
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("tomtom.basic"), tr("Basic"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing map type 'Hybrid map'
    mapTypes << QGeoMapType(QGeoMapType::HybridMap, QStringLiteral("tomtom.hybrid"), tr("Hybrid"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map containing only labels
    mapTypes << QGeoMapType(QGeoMapType::TransitMap, QStringLiteral("tomtom.labels"), tr("Labels"), false, false, mapTypes.size() + 1, pluginName, cameraCaps);

    //: Noun describing map type 'Street map' in dark style
    mapTypes << QGeoMapType(QGeoMapType::StreetMap, QStringLiteral("tomtom.basic-dark"), tr("Basic Dark"), false, true, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing map type 'Hybrid map' in dark style
    mapTypes << QGeoMapType(QGeoMapType::HybridMap, QStringLiteral("tomtom.hybrid-dark"), tr("Hybrid Dark"), false, true, mapTypes.size() + 1, pluginName, cameraCaps);
    //: Noun describing type of a map containing only labels in dark style
    mapTypes << QGeoMapType(QGeoMapType::TransitMap, QStringLiteral("tomtom.labels-dark"), tr("Labels Dark"), false, true, mapTypes.size() + 1, pluginName, cameraCaps);


    QVector<QString> mapIds;
    for (int i=0; i < mapTypes.size(); ++i)
         mapIds.push_back(mapTypes[i].name());

    setSupportedMapTypes(mapTypes);


    int scaleFactor = 1;
    if (parameters.contains(QStringLiteral("tomtom.mapping.highdpi_tiles"))) {
        const QString param = parameters.value(QStringLiteral("tomtom.mapping.highdpi_tiles")).toString().toLower();
        if (param == "true")
            scaleFactor = 2;
    }

    QGeoTileFetcherTomTom *tileFetcher = new QGeoTileFetcherTomTom(scaleFactor, this);
    tileFetcher->setUserAgent(userAgent);
    if (parameters.contains(QStringLiteral("tomtom.access_token"))) {
        const QString token = parameters.value(QStringLiteral("tomtom.access_token")).toString();
        tileFetcher->setAccessToken(token);
    }

    setTileFetcher(tileFetcher);

    // TODO: do this in a plugin-neutral way so that other tiled map plugins
    //       don't need this boilerplate or hardcode plugin name

    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.directory"))) {
        m_cacheDirectory = parameters.value(QStringLiteral("tomtom.mapping.cache.directory")).toString();
    } else {
        // managerName() is not yet set, we have to hardcode the plugin name below
        m_cacheDirectory = QAbstractGeoTileCache::baseLocationCacheDirectory() + QLatin1String(pluginName);
    }

    QGeoFileTileCache *tileCache = new QGeoFileTileCacheTomTom(mapTypes, scaleFactor, m_cacheDirectory);

    /*
     * Disk cache setup -- defaults to Unitary since:
     */
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.disk.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("tomtom.mapping.cache.disk.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyDisk(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyDisk(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyDisk(QGeoFileTileCache::Unitary);
    }
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.disk.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("tomtom.mapping.cache.disk.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setMaxDiskUsage(cacheSize);
    } else {
        if (tileCache->costStrategyDisk() == QGeoFileTileCache::Unitary)
            tileCache->setMaxDiskUsage(20000); // The maximum allowed with the free tier
    }

    /*
     * Memory cache setup -- defaults to ByteSize (old behavior)
     */
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.memory.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("tomtom.mapping.cache.memory.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyMemory(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyMemory(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyMemory(QGeoFileTileCache::Unitary);
    }
    tileCache->setMaxMemoryUsage(0);
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.memory.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("tomtom.mapping.cache.memory.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setMaxMemoryUsage(cacheSize);
    }

    /*
     * Texture cache setup -- defaults to ByteSize (old behavior)
     */
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.texture.cost_strategy"))) {
        QString cacheStrategy = parameters.value(QStringLiteral("tomtom.mapping.cache.texture.cost_strategy")).toString().toLower();
        if (cacheStrategy == QLatin1String("bytesize"))
            tileCache->setCostStrategyTexture(QGeoFileTileCache::ByteSize);
        else
            tileCache->setCostStrategyTexture(QGeoFileTileCache::Unitary);
    } else {
        tileCache->setCostStrategyTexture(QGeoFileTileCache::Unitary);
    }
    tileCache->setExtraTextureUsage(30);
    if (parameters.contains(QStringLiteral("tomtom.mapping.cache.texture.size"))) {
        bool ok = false;
        int cacheSize = parameters.value(QStringLiteral("tomtom.mapping.cache.texture.size")).toString().toInt(&ok);
        if (ok)
            tileCache->setExtraTextureUsage(cacheSize);
    }

    /* PREFETCHING */
    if (parameters.contains(QStringLiteral("tomtom.mapping.prefetching_style"))) {
        const QString prefetchingMode = parameters.value(QStringLiteral("tomtom.mapping.prefetching_style")).toString();
        if (prefetchingMode == QStringLiteral("TwoNeighbourLayers"))
            m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;
        else if (prefetchingMode == QStringLiteral("OneNeighbourLayer"))
            m_prefetchStyle = QGeoTiledMap::PrefetchNeighbourLayer;
        else if (prefetchingMode == QStringLiteral("NoPrefetching"))
            m_prefetchStyle = QGeoTiledMap::NoPrefetching;
    }

    setTileCache(tileCache);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
    QMetaObject::invokeMethod(tileFetcher, "fetchCopyrightsData", Qt::QueuedConnection); // for simplicity, because it has a qnam.
}

QGeoTiledMappingManagerEngineTomTom::~QGeoTiledMappingManagerEngineTomTom()
{
}

QGeoMap *QGeoTiledMappingManagerEngineTomTom::createMap()
{
    QGeoTiledMap *map = new QGeoTiledMapTomTom(this, nullptr);
    map->setPrefetchStyle(m_prefetchStyle);
    return map;
}

void QGeoTiledMappingManagerEngineTomTom::onCopyrightsFetched(const QByteArray &data)
{
    QJsonDocument document = QJsonDocument::fromJson(data);
    qDebug() << document;
}

QT_END_NAMESPACE


