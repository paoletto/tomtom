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

#ifndef QGEOROUTINGMANAGERENGINETOMTOM_H
#define QGEOROUTINGMANAGERENGINETOMTOM_H

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/QGeoRoutingManagerEngine>

QT_BEGIN_NAMESPACE

class QNetworkAccessManager;
class QGeoRouteParser;

class QGeoRoutingManagerEngineTomTom : public QGeoRoutingManagerEngine
{
    Q_OBJECT

public:
    QGeoRoutingManagerEngineTomTom(const QVariantMap &parameters,
                                QGeoServiceProvider::Error *error,
                                QString *errorString);
    ~QGeoRoutingManagerEngineTomTom();

    QGeoRouteReply *calculateRoute(const QGeoRouteRequest &request);
    const QGeoRouteParser *routeParser() const;

    bool m_includeJson = false;
    bool m_debugQuery = false;

private Q_SLOTS:
    void replyFinished();
    void replyError(QGeoRouteReply::Error errorCode, const QString &errorString);

private:
    QNetworkAccessManager *m_networkManager;
    QByteArray m_userAgent;
    QGeoRouteParser *m_routeParser = nullptr;
};

QT_END_NAMESPACE

#endif // QGEOROUTINGMANAGERENGINETOMTOM_H

