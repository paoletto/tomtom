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

#ifndef QTOMTOMCOMMON_H
#define QTOMTOMCOMMON_H

#include <QtCore/QtGlobal>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtPositioning/QGeoLocation>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoAddress>

QT_BEGIN_NAMESPACE

class QTomTomCommon
{
public:
    static void parseAddress(QGeoAddress &addr, const QVariantMap &data)
    {
        // text
        if (data.contains(QLatin1String("freeformAddress")))
            addr.setText( data.value(QLatin1String("freeformAddress")).toString() );
        // country
        if (data.contains(QLatin1String("country")))
            addr.setCountry( data.value(QLatin1String("country")).toString() );
        // countrycode
        if (data.contains(QLatin1String("countryCode")))
            addr.setCountryCode( data.value(QLatin1String("countryCode")).toString() );
        // state
        if (data.contains(QLatin1String("countrySubdivision")))
            addr.setState( data.value(QLatin1String("countrySubdivision")).toString() );
        // county
        if (data.contains(QLatin1String("countrySecondarySubdivision")))
            addr.setCounty( data.value(QLatin1String("countrySecondarySubdivision")).toString() );
        // city
        if (data.contains(QLatin1String("municipality")))
            addr.setCity( data.value(QLatin1String("municipality")).toString() );
        // district
        if (data.contains(QLatin1String("municipalitySubdivision")))
            addr.setDistrict( data.value(QLatin1String("municipalitySubdivision")).toString() );
        // postalcode
        if (data.contains(QLatin1String("postalCode")))
            addr.setPostalCode( data.value(QLatin1String("postalCode")).toString() );
        // street
        if (data.contains(QLatin1String("streetNameAndNumber")))
            addr.setStreet( data.value(QLatin1String("streetNameAndNumber")).toString() );
        else if (data.contains(QLatin1String("streetName")) && data.contains(QLatin1String("streetNumber")))
            addr.setStreet( data.value(QLatin1String("streetName")).toString()
                            + QLatin1String(" ")
                            + data.value(QLatin1String("streetNumber")).toString() ); // Note, the order might be country-dependent.
    }

    inline static QString missingAccessToken()
    {
        return QObject::tr("TomTom plugin requires a 'tomtom.access_token' parameter.\n"
                           "Visit https://developer.tomtom.com/ to obtain an access token");
    }

    static QString toString(const QGeoCoordinate &crd)
    {
        return QString::number(crd.latitude(), 'f', 10) + ',' + QString::number(crd.longitude(), 'f', 10);
    }

    static QGeoCoordinate fromLatLon(const QVariantMap &data)
    {
        return QGeoCoordinate(data.value(QLatin1String("lat")).toDouble(),
                              data.value(QLatin1String("lon")).toDouble());
    }

    inline static const QByteArray userAgent = QByteArrayLiteral("QtLocation TomTom geoservice " QT_VERSION_STR);
    inline static const QByteArray baseUrl = QByteArrayLiteral("https://api.tomtom.com");
    inline static const QByteArray versionNumberGeocoding = QByteArrayLiteral("2");
    inline static const QByteArray baseUrlGeocoding = baseUrl + QByteArrayLiteral("/search/") + versionNumberGeocoding + QByteArrayLiteral("/geocode/");
    inline static const QByteArray baseUrlStructuredGeocoding = baseUrl + QByteArrayLiteral("/search/") + versionNumberGeocoding + QByteArrayLiteral("/structuredGeocode.json");
    inline static const QByteArray baseUrlReverseGeocoding = baseUrl + QByteArrayLiteral("/search/") + versionNumberGeocoding + QByteArrayLiteral("/reverseGeocode/");

    inline static const QByteArray versionNumberMapping = QByteArrayLiteral("1");
    inline static const QByteArray baseUrlMapping = baseUrl  + QByteArrayLiteral("/map/") + versionNumberMapping + QByteArrayLiteral("/tile/");
    inline static const QByteArray baseUrlCopyrights = baseUrl  + QByteArrayLiteral("/map/") + versionNumberMapping + QByteArrayLiteral("/copyrights");
    inline static const QVector<QByteArray> baseUrlMappingPrefixed {
                      QByteArrayLiteral("https://a.api.tomtom.com/map/") + versionNumberMapping + QByteArrayLiteral("/tile/")
                     ,QByteArrayLiteral("https://b.api.tomtom.com/map/") + versionNumberMapping + QByteArrayLiteral("/tile/")
                     ,QByteArrayLiteral("https://c.api.tomtom.com/map/") + versionNumberMapping + QByteArrayLiteral("/tile/")
                     ,QByteArrayLiteral("https://d.api.tomtom.com/map/") + versionNumberMapping + QByteArrayLiteral("/tile/")
    };

    inline static const QByteArray versionNumberRouting = QByteArrayLiteral("1");
    inline static const QByteArray baseUrlRouting = baseUrl + QByteArrayLiteral("/routing/") + versionNumberRouting + QByteArrayLiteral("/calculateRoute/");

    inline static const QByteArray versionNumberPlaces = QByteArrayLiteral("2");
    inline static const QByteArray baseUrlPlaces = baseUrl + QByteArrayLiteral("/search/") + versionNumberPlaces + QByteArrayLiteral("/poiSearch/");
    inline static const QByteArray baseUrlCategories = baseUrl + QByteArrayLiteral("/search/") + versionNumberPlaces + QByteArrayLiteral("/poiCategories.json");

};

QT_END_NAMESPACE

#endif // QTOMTOMCOMMON_H
