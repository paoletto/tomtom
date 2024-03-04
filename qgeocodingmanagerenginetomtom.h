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

#ifndef QGEOCODINGMANAGERENGINETOMTOM_H
#define QGEOCODINGMANAGERENGINETOMTOM_H

#include <QtCore/QUrlQuery>
#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/QGeoCodingManagerEngine>
#include <QtLocation/QGeoCodeReply>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;

class QGeoCodingManagerEngineTomTom : public QGeoCodingManagerEngine
{
    Q_OBJECT

public:
    QGeoCodingManagerEngineTomTom(const QVariantMap &parameters, QGeoServiceProvider::Error *error,
                               QString *errorString);
    ~QGeoCodingManagerEngineTomTom();

    QGeoCodeReply *geocode(const QGeoAddress &address, const QGeoShape &bounds) override;
    QGeoCodeReply *geocode(const QString &address, int limit, int offset,
                           const QGeoShape &bounds) override;
    QGeoCodeReply *reverseGeocode(const QGeoCoordinate &coordinate,
                                  const QGeoShape &bounds) override;

private slots:
    void onReplyFinished();
    void onReplyError(QGeoCodeReply::Error errorCode, const QString &errorString);

private:
    QGeoCodeReply *submitGeocodeRequest(const QUrl &url);

    QNetworkAccessManager *m_networkManager;
    QByteArray m_userAgent;
    QByteArray m_language;
    QString m_accessToken;
};

QT_END_NAMESPACE

#endif // QGEOCODINGMANAGERENGINETOMTOM_H
