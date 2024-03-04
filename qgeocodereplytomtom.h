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

#ifndef QGEOCODEREPLYMAPBOX_H
#define QGEOCODEREPLYMAPBOX_H

#include <QtNetwork/QNetworkReply>
#include <QtLocation/QGeoCodeReply>
#include <QtLocation/private/qgeocodereply_p.h>

QT_BEGIN_NAMESPACE

class QGeoCodeReplyTomTom : public QGeoCodeReply
{
    Q_OBJECT

public:
    explicit QGeoCodeReplyTomTom(QNetworkReply *reply, QObject *parent = 0, const QGeoShape &bounds = QGeoShape());
    ~QGeoCodeReplyTomTom();

private Q_SLOTS:
    void onNetworkReplyFinished();
    void onNetworkReplyError(QNetworkReply::NetworkError error);
};

class QGeoCodeReplyTomTomPrivate : public QGeoCodeReplyPrivate
{
public:
    QGeoCodeReplyTomTomPrivate(const QGeoShape &bounds);
    ~QGeoCodeReplyTomTomPrivate();
    QVariantMap extraData() const override;

    QVariantMap m_extraData;
    QGeoShape m_bounds;
};


QT_END_NAMESPACE

#endif // QGEOCODEREPLYMAPBOX_H
