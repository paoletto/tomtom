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

#include "qgeomapreplytomtom.h"

#include <QtLocation/private/qgeotilespec_p.h>

QGeoMapReplyTomTom::QGeoMapReplyTomTom(QNetworkReply *reply, const QGeoTileSpec &spec, QObject *parent)
:   QGeoTiledMapReply(spec, parent)
{
    if (!reply) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;** Contact: https://www.qt.io/licensing/
    }
    connect(reply, SIGNAL(finished()), this, SLOT(networkReplyFinished()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(networkReplyError(QNetworkReply::NetworkError)));
    connect(this, &QGeoTiledMapReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QGeoMapReplyTomTom::~QGeoMapReplyTomTom()
{
}

void QGeoMapReplyTomTom::networkReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    // ToDo: honor tile expiration? it comes with reply->rawHeader("Expires"). However, they usually
    // last one day. Which is typically too little for a cache to be really useful.
    // Ideally even for cached tiles requests should be fired to verify that the remote resource hasn't changed.
    // In other words reply->rawHeader("ETag") should be fetched and stored, and used in tile requests.
    // But this requires a redesign of the tilecache.
    setMapImageData(reply->readAll());
    setMapImageFormat(QByteArrayLiteral("png"));
    setFinished(true);
}

void QGeoMapReplyTomTom::networkReplyError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (error == QNetworkReply::OperationCanceledError)
        setFinished(true);
    else
        setError(QGeoTiledMapReply::CommunicationError, reply->errorString());
}
