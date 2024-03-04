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

#include "qgeoserviceproviderplugintomtom.h"
#include "qgeocodingmanagerenginetomtom.h"
#include "qgeotiledmappingmanagerenginetomtom.h"
#include "qgeoroutingmanagerenginetomtom.h"
#include "qplacemanagerenginetomtom.h"
#include "qtomtomcommon.h"

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QT_BEGIN_NAMESPACE


QGeoMappingManagerEngine *QGeoServiceProviderFactoryTomTom::createMappingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    const QString accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    if (!accessToken.isEmpty()) {
        return new QGeoTiledMappingManagerEngineTomTom(parameters, error, errorString);
    } else {
        *error = QGeoServiceProvider::MissingRequiredParameterError;
        *errorString = QTomTomCommon::missingAccessToken();
        return nullptr;
    }
}

QGeoCodingManagerEngine *QGeoServiceProviderFactoryTomTom::createGeocodingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    const QString accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    if (!accessToken.isEmpty()) {
        return new QGeoCodingManagerEngineTomTom(parameters, error, errorString);
    } else {
        *error = QGeoServiceProvider::MissingRequiredParameterError;
        *errorString = QTomTomCommon::missingAccessToken();
        return nullptr;
    }
}

QGeoRoutingManagerEngine *QGeoServiceProviderFactoryTomTom::createRoutingManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    const QString accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    if (!accessToken.isEmpty()) {
        return new QGeoRoutingManagerEngineTomTom(parameters, error, errorString);
    } else {
        *error = QGeoServiceProvider::MissingRequiredParameterError;
        *errorString = QTomTomCommon::missingAccessToken();
        return nullptr;
    }
}

QPlaceManagerEngine *QGeoServiceProviderFactoryTomTom::createPlaceManagerEngine(
    const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString) const
{
    const QString accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    if (!accessToken.isEmpty()) {
        return new QPlaceManagerEngineTomTom(parameters, error, errorString);
    } else {
        *error = QGeoServiceProvider::MissingRequiredParameterError;
        *errorString = QTomTomCommon::missingAccessToken();
        return nullptr;
    }
}

QT_END_NAMESPACE
