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

#include "qgeoroutereplytomtom.h"
#include "qgeoroutingmanagerenginetomtom.h"
#include <QtLocation/private/qgeorouteparser_p.h>
#include <QtLocation/private/qgeoroute_p.h>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtLocation/QGeoRouteSegment>
#include <QtLocation/QGeoManeuver>

namespace {

class QGeoRouteTomTom : public QGeoRoute
{
public:
    QGeoRouteTomTom(const QGeoRoute &other, const QVariantMap &metadata);
};

class QGeoRoutePrivateTomTom : public QGeoRoutePrivateDefault
{
public:
    QGeoRoutePrivateTomTom(const QGeoRoutePrivateDefault &other, const QVariantMap &metadata);

    virtual QString engineName() const override;
    virtual QVariantMap metadata() const override;

    QVariantMap m_metadata;
};

QGeoRouteTomTom::QGeoRouteTomTom(const QGeoRoute &other, const QVariantMap &metadata)
    : QGeoRoute(QExplicitlySharedDataPointer<QGeoRoutePrivateTomTom>(new QGeoRoutePrivateTomTom(*static_cast<const QGeoRoutePrivateDefault *>(QGeoRoutePrivate::routePrivateData(other)), metadata)))
{
}

QGeoRoutePrivateTomTom::QGeoRoutePrivateTomTom(const QGeoRoutePrivateDefault &other, const QVariantMap &metadata)
    : QGeoRoutePrivateDefault(other)
    , m_metadata(metadata)
{
}

QString QGeoRoutePrivateTomTom::engineName() const
{
    return QLatin1String("tomtom");
}

QVariantMap QGeoRoutePrivateTomTom::metadata() const
{
    return m_metadata;
}

} // namespace

QT_BEGIN_NAMESPACE

QGeoRouteReplyTomTom::QGeoRouteReplyTomTom(QNetworkReply *reply,
                                            const QGeoRouteRequest &request,
                                            QObject *parent)
:   QGeoRouteReply(request, parent)
{
    if (!reply) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }
    connect(reply, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onNetworkReplyError(QNetworkReply::NetworkError)));
    connect(this, &QGeoRouteReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QGeoRouteReplyTomTom::~QGeoRouteReplyTomTom()
{
}

void QGeoRouteReplyTomTom::onNetworkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    QGeoRoutingManagerEngineTomTom *engine = qobject_cast<QGeoRoutingManagerEngineTomTom *>(parent());
    const QGeoRouteParser *parser = engine->routeParser();

    QList<QGeoRoute> routes;
    QString errorString;

    QByteArray routeReply = reply->readAll();
    QGeoRouteReply::Error error = parser->parseReply(routes, errorString, routeReply, request());
    // Setting the request into the result
    for (QGeoRoute &route : routes) {
        route.setRequest(request());
        for (QGeoRoute &leg: route.routeLegs()) {
            leg.setRequest(request());
        }
    }

    QVariantMap metadata;
    if (engine->m_includeJson)
        metadata["tomtom.reply_json"] = routeReply;
    if (engine->m_debugQuery)
        metadata["tomtom.query_url"] = reply->url();

    QList<QGeoRoute> georoutes;
    for (const QGeoRoute &route : routes.mid(0, request().numberAlternativeRoutes() + 1)) {
        QGeoRouteTomTom georoute(route, metadata);
        georoutes.append(georoute);
    }

    if (error == QGeoRouteReply::NoError) {
        setRoutes(georoutes);
        // setError(QGeoRouteReply::NoError, status);  // can't do this, or NoError is emitted and does damages
        setFinished(true);
    } else {
        setError(error, errorString);
    }
}

void QGeoRouteReplyTomTom::onNetworkReplyError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    setError(QGeoRouteReply::CommunicationError, reply->errorString());
}

QT_END_NAMESPACE
