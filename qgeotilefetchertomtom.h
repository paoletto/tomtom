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

#ifndef QGEOTILEFETCHERTOMTOM_H
#define QGEOTILEFETCHERTOMTOM_H

#include <qvector.h>
#include <QtLocation/private/qgeotilefetcher_p.h>

QT_BEGIN_NAMESPACE

class QGeoTiledMappingManagerEngineTomTom;
class QNetworkAccessManager;
class QNetworkReply;

class QGeoTileFetcherTomTom : public QGeoTileFetcher
{
    Q_OBJECT

public:
    QGeoTileFetcherTomTom(int scaleFactor, QGeoTiledMappingManagerEngineTomTom *parent);

    void setUserAgent(const QByteArray &userAgent);
    void setAccessToken(const QString &accessToken);

public Q_SLOTS:
    void onCopyrightsFetched();
    void fetchCopyrightsData();

private:
    QGeoTiledMapReply *getTileImage(const QGeoTileSpec &spec);

    QGeoTiledMappingManagerEngineTomTom *m_engine;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_copyrightsReply = nullptr;
    QByteArray m_userAgent;
    QByteArray m_format = QByteArrayLiteral("png");
    QByteArray m_replyFormat = QByteArrayLiteral("png");
    QByteArray m_accessToken;
    QByteArray m_language;
    quint64 m_fetchedTiles = 0;
    int m_scaleFactor;
};

QT_END_NAMESPACE

#endif // QGEOTILEFETCHERTOMTOM_H
