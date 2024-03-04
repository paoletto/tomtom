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

#include "qgeocodereplytomtom.h"
#include "qtomtomcommon.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoAddress>
#include <QtPositioning/QGeoLocation>
#include <QtPositioning/QGeoRectangle>

QT_BEGIN_NAMESPACE

QGeoCodeReplyTomTom::QGeoCodeReplyTomTom(QNetworkReply *reply, QObject *parent, const QGeoShape &bounds)
:   QGeoCodeReply(*new QGeoCodeReplyTomTomPrivate(bounds), parent)
{
    Q_ASSERT(parent);
    if (!reply) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }

    connect(reply, &QNetworkReply::finished, this, &QGeoCodeReplyTomTom::onNetworkReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &QGeoCodeReplyTomTom::onNetworkReplyError);

    connect(this, &QGeoCodeReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QGeoCodeReplyTomTom::~QGeoCodeReplyTomTom()
{
}

static QGeoCoordinate fromString(const QString &posString)
{
    QGeoCoordinate res;
    const QStringList latlon = posString.split(',');
    if (latlon.size() != 2)
        return res;

    res.setLatitude(latlon.first().toDouble());
    res.setLatitude(latlon.last().toDouble());
    return res;
}

static void parseGeocodeResult(QList<QGeoLocation> &locations, const QVariantList &results)
{
    for (const QVariant &res: results) {
        QGeoLocation loc;
        QGeoAddress address;
        const QVariantMap data = res.toMap();
        const QVariantMap addressData = data.value(QLatin1String("address")).toMap();
        const QVariantMap positionData = data.value(QLatin1String("position")).toMap();
        loc.setCoordinate(QTomTomCommon::fromLatLon(positionData));

        QGeoRectangle bbox;
        if (data.contains(QLatin1String("viewport"))) {
            const QVariantMap bboxData = data.value(QLatin1String("viewport")).toMap();
            const QVariantMap tl = bboxData.value(QLatin1String("topLeftPoint")).toMap();
            const QVariantMap br = bboxData.value(QLatin1String("btmRightPoint")).toMap();
            bbox.setTopLeft(QTomTomCommon::fromLatLon(tl));
            bbox.setBottomRight(QTomTomCommon::fromLatLon(br));
        }
        loc.setBoundingBox(bbox);
        QTomTomCommon::parseAddress(address, addressData);
        loc.setAddress(address);
        locations.append(loc);
    }
}

static void parseReverseGeocodeResult(QList<QGeoLocation> &locations, const QVariantList &results)
{
    for (const QVariant &res: results) {
        QGeoLocation loc;
        QGeoAddress address;
        const QVariantMap data = res.toMap();
        const QVariantMap addressData = data.value(QLatin1String("address")).toMap();
        loc.setCoordinate(fromString(data.value(QLatin1String("position")).toString()));

        QGeoRectangle bbox;
        if (addressData.contains(QLatin1String("boundingBox"))) {
            const QVariantMap bboxData = addressData.value(QLatin1String("boundingBox")).toMap();
            bbox.setTopLeft(fromString(bboxData.value(QLatin1String("northEast")).toString()));
            bbox.setBottomRight(fromString(bboxData.value(QLatin1String("southWest")).toString()));
        }
        if (bbox.isValid())
            loc.setBoundingBox(bbox);

        QVariantMap extra;
        if (data.contains(QLatin1String("score")))
            extra[QLatin1String("score")] = data.value(QLatin1String("score"));
        if (data.contains(QLatin1String("speedLimit")))
            extra[QLatin1String("speedLimit")] = data.value(QLatin1String("speedLimit"));

        QTomTomCommon::parseAddress(address, addressData);
        loc.setAddress(address);
        loc.setExtendedAttributes(extra);
        locations.append(loc);
    }
}

void QGeoCodeReplyTomTom::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    QList<QGeoLocation> locations;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (Q_UNLIKELY(!document.isObject())) {
        setError(ParseError, tr("Response parse error"));
        return;
    }

    const QVariantMap response = document.object().toVariantMap();
    const QVariantMap summary = response.value(QLatin1String("summary")).toMap();

    const bool reverse = response.contains(QLatin1String("addresses"));
    QVariantList results;
    if (reverse) {
        results = response.value(QLatin1String("addresses")).toList();
        parseReverseGeocodeResult(locations, results);
    } else {
        results = response.value(QLatin1String("results")).toList();
        parseGeocodeResult(locations, results);
    }
    //const int totalResults = summary.value(QLatin1String("totalResults")).toInt();

//    QGeoCodeReplyTomTomPrivate *replyPrivate
//            = static_cast<QGeoCodeReplyTomTomPrivate *>(QGeoCodeReplyPrivate::get(*this));
//    const QGeoShape &bounds = replyPrivate->m_bounds;
//    if (bounds.isValid()) {
//        // Sort
//    }

    qDebug() << document;

    setLocations(locations);
    setFinished(true);
}

void QGeoCodeReplyTomTom::onNetworkReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    setError(QGeoCodeReply::CommunicationError, reply->errorString());
}

QGeoCodeReplyTomTomPrivate::QGeoCodeReplyTomTomPrivate(const QGeoShape &bounds) : m_bounds(bounds)
{

}

QGeoCodeReplyTomTomPrivate::~QGeoCodeReplyTomTomPrivate()
{

}

QVariantMap QGeoCodeReplyTomTomPrivate::extraData() const
{
    return m_extraData;
}

QT_END_NAMESPACE
