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

#include "qgeotilefetchertomtom.h"
#include "qgeotiledmappingmanagerenginetomtom.h"
#include "qgeomapreplytomtom.h"
#include "qtomtomcommon.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

static const QVector<QByteArray> layers{
    QByteArrayLiteral("basic/"),
    QByteArrayLiteral("hybrid/"),
    QByteArrayLiteral("labels/"),
    QByteArrayLiteral("basic/"),
    QByteArrayLiteral("hybrid/"),
    QByteArrayLiteral("labels/")
};
static const QVector<QByteArray> styles{
    QByteArrayLiteral("main/"),
    QByteArrayLiteral("main/"),
    QByteArrayLiteral("main/"),
    QByteArrayLiteral("night/"),
    QByteArrayLiteral("night/"),
    QByteArrayLiteral("night/")
};

static const QSet<QByteArray> acceptedLanguages {
    "ar", "bg-BG", "zh-TW", "cs-CZ", "da-DK", "nl-NL", "en-AU", "en-CA", "en-GB", "en-NZ", "en-US", "fi-FI", "fr-FR", "de-DE", "el-GR", "hu-HU", "id-ID", "it-IT", "ko-KR", "lt-LT", "ms-MY", "nb-NO", "pl-PL", "pt-BR", "pt-PT", "ru-RU", "ru-Latn-RU", "ru-Cyrl-RU", "sk-SK", "sl-SL", "es-ES", "es-MX", "sv-SE", "th-TH", "tr-TR"
};

QGeoTileFetcherTomTom::QGeoTileFetcherTomTom(int scaleFactor, QGeoTiledMappingManagerEngineTomTom *parent)
:   QGeoTileFetcher(parent),
    m_engine(parent),
    m_networkManager(new QNetworkAccessManager(this)),
    m_userAgent(QTomTomCommon::userAgent)
{
    m_scaleFactor = qBound(1, scaleFactor, 2);
    m_language = m_engine->locale().name().toLatin1();
    if (!acceptedLanguages.contains(m_language))
        m_language = "NGT-Latn";
}

void QGeoTileFetcherTomTom::setUserAgent(const QByteArray &userAgent)
{
    m_userAgent = userAgent;
}

void QGeoTileFetcherTomTom::setAccessToken(const QString &accessToken)
{
    m_accessToken = accessToken.toLatin1();
}

void QGeoTileFetcherTomTom::onCopyrightsFetched()
{
    if (!m_copyrightsReply)
        return;

    m_copyrightsReply->deleteLater();
    if (m_engine && m_copyrightsReply->error() == QNetworkReply::NoError) {
        QMetaObject::invokeMethod(m_engine,
                                  "onCopyrightsFetched",
                                  Qt::QueuedConnection,
                                  Q_ARG(const QByteArray &, m_copyrightsReply->readAll()));
    }

    m_copyrightsReply = nullptr;;
}

void QGeoTileFetcherTomTom::fetchCopyrightsData()
{
    // ToDo: clarify whether copyrights calls have an impact on plans. until then use the default copyright
    // for the whole world
    return;
    QByteArray copyrightUrl = QTomTomCommon::baseUrlCopyrights + QByteArrayLiteral(".json?key=");
    copyrightUrl += m_accessToken;
    QNetworkRequest request;
    request.setUrl(QUrl(copyrightUrl));
    m_copyrightsReply = m_networkManager->get(request);

    if (m_copyrightsReply->isFinished())
        onCopyrightsFetched();
    else
        connect(m_copyrightsReply, SIGNAL(finished()), this, SLOT(onCopyrightsFetched()));
}

QGeoTiledMapReply *QGeoTileFetcherTomTom::getTileImage(const QGeoTileSpec &spec)
{
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    const int mapId = qBound(0, spec.mapId() - 1, styles.size() - 1);
    QByteArray url = QTomTomCommon::baseUrlMappingPrefixed.at(
                m_fetchedTiles++ % QTomTomCommon::baseUrlMappingPrefixed.size());
    url += layers.at(mapId);
    url += styles.at(mapId);
    url += QString::number(spec.zoom()).toLatin1() + QLatin1Char('/');
    url += QString::number(spec.x()).toLatin1() + QLatin1Char('/');
    url += QString::number(spec.y()).toLatin1() + QLatin1Char('.') + m_format;
    url += QByteArrayLiteral("?key=") + m_accessToken;
    url += QByteArrayLiteral("&language=") + m_language;
    // ToDo: support "political views"
    url += QByteArrayLiteral("&tileSize=") + ((m_scaleFactor > 1) ? QByteArrayLiteral("512") : QByteArrayLiteral("256"));
    request.setUrl(QUrl(url));

    QNetworkReply *reply = m_networkManager->get(request);

    return new QGeoMapReplyTomTom(reply, spec);
}

QT_END_NAMESPACE
