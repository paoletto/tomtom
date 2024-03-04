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

#include "qgeocodingmanagerenginetomtom.h"
#include "qgeocodereplytomtom.h"
#include "qtomtomcommon.h"

#include <QtCore/QVariantMap>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtPositioning/QGeoCoordinate>
#include <QtPositioning/QGeoAddress>
#include <QtPositioning/QGeoShape>
#include <QtPositioning/QGeoCircle>
#include <QtPositioning/QGeoRectangle>

QT_BEGIN_NAMESPACE

namespace {
    static const QString allAddressTypes = QStringLiteral("address,district,locality,neighborhood,place,postcode,region,country");

    // Structured address search requires 2 letters country code.
    static const QMap<QLatin1String, QLatin1String> countryCodes {
        { QLatin1String("andorra")  ,  QLatin1String("AD") },
        { QLatin1String("united arab emirates")  ,  QLatin1String("AE") },
        { QLatin1String("afghanistan")  ,  QLatin1String("AF") },
        { QLatin1String("antigua and barbuda")  ,  QLatin1String("AG") },
        { QLatin1String("anguilla")  ,  QLatin1String("AI") },
        { QLatin1String("albania")  ,  QLatin1String("AL") },
        { QLatin1String("armenia")  ,  QLatin1String("AM") },
        { QLatin1String("angola")  ,  QLatin1String("AO") },
        { QLatin1String("antarctica")  ,  QLatin1String("AQ") },
        { QLatin1String("argentina")  ,  QLatin1String("AR") },
        { QLatin1String("american samoa")  ,  QLatin1String("AS") },
        { QLatin1String("austria")  ,  QLatin1String("AT") },
        { QLatin1String("australia")  ,  QLatin1String("AU") },
        { QLatin1String("aruba")  ,  QLatin1String("AW") },
        { QLatin1String("aland islands")  ,  QLatin1String("AX") },
        { QLatin1String("azerbaijan")  ,  QLatin1String("AZ") },
        { QLatin1String("bosnia and herzegovina")  ,  QLatin1String("BA") },
        { QLatin1String("barbados")  ,  QLatin1String("BB") },
        { QLatin1String("bangladesh")  ,  QLatin1String("BD") },
        { QLatin1String("belgium")  ,  QLatin1String("BE") },
        { QLatin1String("burkina faso")  ,  QLatin1String("BF") },
        { QLatin1String("bulgaria")  ,  QLatin1String("BG") },
        { QLatin1String("bahrain")  ,  QLatin1String("BH") },
        { QLatin1String("burundi")  ,  QLatin1String("BI") },
        { QLatin1String("benin")  ,  QLatin1String("BJ") },
        { QLatin1String("saint barthelemy")  ,  QLatin1String("BL") },
        { QLatin1String("bermuda")  ,  QLatin1String("BM") },
        { QLatin1String("brunei")  ,  QLatin1String("BN") },
        { QLatin1String("bolivia")  ,  QLatin1String("BO") },
        { QLatin1String("bonaire, saint eustatius and saba ")  ,  QLatin1String("BQ") },
        { QLatin1String("brazil")  ,  QLatin1String("BR") },
        { QLatin1String("bahamas")  ,  QLatin1String("BS") },
        { QLatin1String("bhutan")  ,  QLatin1String("BT") },
        { QLatin1String("bouvet island")  ,  QLatin1String("BV") },
        { QLatin1String("botswana")  ,  QLatin1String("BW") },
        { QLatin1String("belarus")  ,  QLatin1String("BY") },
        { QLatin1String("belize")  ,  QLatin1String("BZ") },
        { QLatin1String("canada")  ,  QLatin1String("CA") },
        { QLatin1String("cocos islands")  ,  QLatin1String("CC") },
        { QLatin1String("democratic republic of the congo")  ,  QLatin1String("CD") },
        { QLatin1String("central african republic")  ,  QLatin1String("CF") },
        { QLatin1String("republic of the congo")  ,  QLatin1String("CG") },
        { QLatin1String("switzerland")  ,  QLatin1String("CH") },
        { QLatin1String("ivory coast")  ,  QLatin1String("CI") },
        { QLatin1String("cook islands")  ,  QLatin1String("CK") },
        { QLatin1String("chile")  ,  QLatin1String("CL") },
        { QLatin1String("cameroon")  ,  QLatin1String("CM") },
        { QLatin1String("china")  ,  QLatin1String("CN") },
        { QLatin1String("colombia")  ,  QLatin1String("CO") },
        { QLatin1String("costa rica")  ,  QLatin1String("CR") },
        { QLatin1String("cuba")  ,  QLatin1String("CU") },
        { QLatin1String("cape verde")  ,  QLatin1String("CV") },
        { QLatin1String("curacao")  ,  QLatin1String("CW") },
        { QLatin1String("christmas island")  ,  QLatin1String("CX") },
        { QLatin1String("cyprus")  ,  QLatin1String("CY") },
        { QLatin1String("czech republic")  ,  QLatin1String("CZ") },
        { QLatin1String("germany")  ,  QLatin1String("DE") },
        { QLatin1String("djibouti")  ,  QLatin1String("DJ") },
        { QLatin1String("denmark")  ,  QLatin1String("DK") },
        { QLatin1String("dominica")  ,  QLatin1String("DM") },
        { QLatin1String("dominican republic")  ,  QLatin1String("DO") },
        { QLatin1String("algeria")  ,  QLatin1String("DZ") },
        { QLatin1String("ecuador")  ,  QLatin1String("EC") },
        { QLatin1String("estonia")  ,  QLatin1String("EE") },
        { QLatin1String("egypt")  ,  QLatin1String("EG") },
        { QLatin1String("western sahara")  ,  QLatin1String("EH") },
        { QLatin1String("eritrea")  ,  QLatin1String("ER") },
        { QLatin1String("spain")  ,  QLatin1String("ES") },
        { QLatin1String("ethiopia")  ,  QLatin1String("ET") },
        { QLatin1String("finland")  ,  QLatin1String("FI") },
        { QLatin1String("fiji")  ,  QLatin1String("FJ") },
        { QLatin1String("falkland islands")  ,  QLatin1String("FK") },
        { QLatin1String("micronesia")  ,  QLatin1String("FM") },
        { QLatin1String("faroe islands")  ,  QLatin1String("FO") },
        { QLatin1String("france")  ,  QLatin1String("FR") },
        { QLatin1String("gabon")  ,  QLatin1String("GA") },
        { QLatin1String("united kingdom")  ,  QLatin1String("GB") },
        { QLatin1String("grenada")  ,  QLatin1String("GD") },
        { QLatin1String("georgia")  ,  QLatin1String("GE") },
        { QLatin1String("french guiana")  ,  QLatin1String("GF") },
        { QLatin1String("guernsey")  ,  QLatin1String("GG") },
        { QLatin1String("ghana")  ,  QLatin1String("GH") },
        { QLatin1String("gibraltar")  ,  QLatin1String("GI") },
        { QLatin1String("greenland")  ,  QLatin1String("GL") },
        { QLatin1String("gambia")  ,  QLatin1String("GM") },
        { QLatin1String("guinea")  ,  QLatin1String("GN") },
        { QLatin1String("guadeloupe")  ,  QLatin1String("GP") },
        { QLatin1String("equatorial guinea")  ,  QLatin1String("GQ") },
        { QLatin1String("greece")  ,  QLatin1String("GR") },
        { QLatin1String("south georgia and the south sandwich islands")  ,  QLatin1String("GS") },
        { QLatin1String("guatemala")  ,  QLatin1String("GT") },
        { QLatin1String("guam")  ,  QLatin1String("GU") },
        { QLatin1String("guinea-bissau")  ,  QLatin1String("GW") },
        { QLatin1String("guyana")  ,  QLatin1String("GY") },
        { QLatin1String("hong kong")  ,  QLatin1String("HK") },
        { QLatin1String("heard island and mcdonald islands")  ,  QLatin1String("HM") },
        { QLatin1String("honduras")  ,  QLatin1String("HN") },
        { QLatin1String("croatia")  ,  QLatin1String("HR") },
        { QLatin1String("haiti")  ,  QLatin1String("HT") },
        { QLatin1String("hungary")  ,  QLatin1String("HU") },
        { QLatin1String("indonesia")  ,  QLatin1String("ID") },
        { QLatin1String("ireland")  ,  QLatin1String("IE") },
        { QLatin1String("israel")  ,  QLatin1String("IL") },
        { QLatin1String("isle of man")  ,  QLatin1String("IM") },
        { QLatin1String("india")  ,  QLatin1String("IN") },
        { QLatin1String("british indian ocean territory")  ,  QLatin1String("IO") },
        { QLatin1String("iraq")  ,  QLatin1String("IQ") },
        { QLatin1String("iran")  ,  QLatin1String("IR") },
        { QLatin1String("iceland")  ,  QLatin1String("IS") },
        { QLatin1String("italy")  ,  QLatin1String("IT") },
        { QLatin1String("jersey")  ,  QLatin1String("JE") },
        { QLatin1String("jamaica")  ,  QLatin1String("JM") },
        { QLatin1String("jordan")  ,  QLatin1String("JO") },
        { QLatin1String("japan")  ,  QLatin1String("JP") },
        { QLatin1String("kenya")  ,  QLatin1String("KE") },
        { QLatin1String("kyrgyzstan")  ,  QLatin1String("KG") },
        { QLatin1String("cambodia")  ,  QLatin1String("KH") },
        { QLatin1String("kiribati")  ,  QLatin1String("KI") },
        { QLatin1String("comoros")  ,  QLatin1String("KM") },
        { QLatin1String("saint kitts and nevis")  ,  QLatin1String("KN") },
        { QLatin1String("north korea")  ,  QLatin1String("KP") },
        { QLatin1String("south korea")  ,  QLatin1String("KR") },
        { QLatin1String("kuwait")  ,  QLatin1String("KW") },
        { QLatin1String("cayman islands")  ,  QLatin1String("KY") },
        { QLatin1String("kazakhstan")  ,  QLatin1String("KZ") },
        { QLatin1String("laos")  ,  QLatin1String("LA") },
        { QLatin1String("lebanon")  ,  QLatin1String("LB") },
        { QLatin1String("saint lucia")  ,  QLatin1String("LC") },
        { QLatin1String("liechtenstein")  ,  QLatin1String("LI") },
        { QLatin1String("sri lanka")  ,  QLatin1String("LK") },
        { QLatin1String("liberia")  ,  QLatin1String("LR") },
        { QLatin1String("lesotho")  ,  QLatin1String("LS") },
        { QLatin1String("lithuania")  ,  QLatin1String("LT") },
        { QLatin1String("luxembourg")  ,  QLatin1String("LU") },
        { QLatin1String("latvia")  ,  QLatin1String("LV") },
        { QLatin1String("libya")  ,  QLatin1String("LY") },
        { QLatin1String("morocco")  ,  QLatin1String("MA") },
        { QLatin1String("monaco")  ,  QLatin1String("MC") },
        { QLatin1String("moldova")  ,  QLatin1String("MD") },
        { QLatin1String("montenegro")  ,  QLatin1String("ME") },
        { QLatin1String("saint martin")  ,  QLatin1String("MF") },
        { QLatin1String("madagascar")  ,  QLatin1String("MG") },
        { QLatin1String("marshall islands")  ,  QLatin1String("MH") },
        { QLatin1String("macedonia")  ,  QLatin1String("MK") },
        { QLatin1String("mali")  ,  QLatin1String("ML") },
        { QLatin1String("myanmar")  ,  QLatin1String("MM") },
        { QLatin1String("mongolia")  ,  QLatin1String("MN") },
        { QLatin1String("macao")  ,  QLatin1String("MO") },
        { QLatin1String("northern mariana islands")  ,  QLatin1String("MP") },
        { QLatin1String("martinique")  ,  QLatin1String("MQ") },
        { QLatin1String("mauritania")  ,  QLatin1String("MR") },
        { QLatin1String("montserrat")  ,  QLatin1String("MS") },
        { QLatin1String("malta")  ,  QLatin1String("MT") },
        { QLatin1String("mauritius")  ,  QLatin1String("MU") },
        { QLatin1String("maldives")  ,  QLatin1String("MV") },
        { QLatin1String("malawi")  ,  QLatin1String("MW") },
        { QLatin1String("mexico")  ,  QLatin1String("MX") },
        { QLatin1String("malaysia")  ,  QLatin1String("MY") },
        { QLatin1String("mozambique")  ,  QLatin1String("MZ") },
        { QLatin1String("namibia")  ,  QLatin1String("NA") },
        { QLatin1String("new caledonia")  ,  QLatin1String("NC") },
        { QLatin1String("niger")  ,  QLatin1String("NE") },
        { QLatin1String("norfolk island")  ,  QLatin1String("NF") },
        { QLatin1String("nigeria")  ,  QLatin1String("NG") },
        { QLatin1String("nicaragua")  ,  QLatin1String("NI") },
        { QLatin1String("netherlands")  ,  QLatin1String("NL") },
        { QLatin1String("norway")  ,  QLatin1String("NO") },
        { QLatin1String("nepal")  ,  QLatin1String("NP") },
        { QLatin1String("nauru")  ,  QLatin1String("NR") },
        { QLatin1String("niue")  ,  QLatin1String("NU") },
        { QLatin1String("new zealand")  ,  QLatin1String("NZ") },
        { QLatin1String("oman")  ,  QLatin1String("OM") },
        { QLatin1String("panama")  ,  QLatin1String("PA") },
        { QLatin1String("peru")  ,  QLatin1String("PE") },
        { QLatin1String("french polynesia")  ,  QLatin1String("PF") },
        { QLatin1String("papua new guinea")  ,  QLatin1String("PG") },
        { QLatin1String("philippines")  ,  QLatin1String("PH") },
        { QLatin1String("pakistan")  ,  QLatin1String("PK") },
        { QLatin1String("poland")  ,  QLatin1String("PL") },
        { QLatin1String("saint pierre and miquelon")  ,  QLatin1String("PM") },
        { QLatin1String("pitcairn")  ,  QLatin1String("PN") },
        { QLatin1String("puerto rico")  ,  QLatin1String("PR") },
        { QLatin1String("palestinian territory")  ,  QLatin1String("PS") },
        { QLatin1String("portugal")  ,  QLatin1String("PT") },
        { QLatin1String("palau")  ,  QLatin1String("PW") },
        { QLatin1String("paraguay")  ,  QLatin1String("PY") },
        { QLatin1String("qatar")  ,  QLatin1String("QA") },
        { QLatin1String("reunion")  ,  QLatin1String("RE") },
        { QLatin1String("romania")  ,  QLatin1String("RO") },
        { QLatin1String("serbia")  ,  QLatin1String("RS") },
        { QLatin1String("russia")  ,  QLatin1String("RU") },
        { QLatin1String("rwanda")  ,  QLatin1String("RW") },
        { QLatin1String("saudi arabia")  ,  QLatin1String("SA") },
        { QLatin1String("solomon islands")  ,  QLatin1String("SB") },
        { QLatin1String("seychelles")  ,  QLatin1String("SC") },
        { QLatin1String("sudan")  ,  QLatin1String("SD") },
        { QLatin1String("sweden")  ,  QLatin1String("SE") },
        { QLatin1String("singapore")  ,  QLatin1String("SG") },
        { QLatin1String("saint helena")  ,  QLatin1String("SH") },
        { QLatin1String("slovenia")  ,  QLatin1String("SI") },
        { QLatin1String("svalbard and jan mayen")  ,  QLatin1String("SJ") },
        { QLatin1String("slovakia")  ,  QLatin1String("SK") },
        { QLatin1String("sierra leone")  ,  QLatin1String("SL") },
        { QLatin1String("san marino")  ,  QLatin1String("SM") },
        { QLatin1String("senegal")  ,  QLatin1String("SN") },
        { QLatin1String("somalia")  ,  QLatin1String("SO") },
        { QLatin1String("suriname")  ,  QLatin1String("SR") },
        { QLatin1String("south sudan")  ,  QLatin1String("SS") },
        { QLatin1String("sao tome and principe")  ,  QLatin1String("ST") },
        { QLatin1String("el salvador")  ,  QLatin1String("SV") },
        { QLatin1String("sint maarten")  ,  QLatin1String("SX") },
        { QLatin1String("syria")  ,  QLatin1String("SY") },
        { QLatin1String("swaziland")  ,  QLatin1String("SZ") },
        { QLatin1String("turks and caicos islands")  ,  QLatin1String("TC") },
        { QLatin1String("chad")  ,  QLatin1String("TD") },
        { QLatin1String("french southern territories")  ,  QLatin1String("TF") },
        { QLatin1String("togo")  ,  QLatin1String("TG") },
        { QLatin1String("thailand")  ,  QLatin1String("TH") },
        { QLatin1String("tajikistan")  ,  QLatin1String("TJ") },
        { QLatin1String("tokelau")  ,  QLatin1String("TK") },
        { QLatin1String("east timor")  ,  QLatin1String("TL") },
        { QLatin1String("turkmenistan")  ,  QLatin1String("TM") },
        { QLatin1String("tunisia")  ,  QLatin1String("TN") },
        { QLatin1String("tonga")  ,  QLatin1String("TO") },
        { QLatin1String("turkey")  ,  QLatin1String("TR") },
        { QLatin1String("trinidad and tobago")  ,  QLatin1String("TT") },
        { QLatin1String("tuvalu")  ,  QLatin1String("TV") },
        { QLatin1String("taiwan")  ,  QLatin1String("TW") },
        { QLatin1String("tanzania")  ,  QLatin1String("TZ") },
        { QLatin1String("ukraine")  ,  QLatin1String("UA") },
        { QLatin1String("uganda")  ,  QLatin1String("UG") },
        { QLatin1String("united states minor outlying islands")  ,  QLatin1String("UM") },
        { QLatin1String("united states")  ,  QLatin1String("US") },
        { QLatin1String("uruguay")  ,  QLatin1String("UY") },
        { QLatin1String("uzbekistan")  ,  QLatin1String("UZ") },
        { QLatin1String("vatican")  ,  QLatin1String("VA") },
        { QLatin1String("saint vincent and the grenadines")  ,  QLatin1String("VC") },
        { QLatin1String("venezuela")  ,  QLatin1String("VE") },
        { QLatin1String("british virgin islands")  ,  QLatin1String("VG") },
        { QLatin1String("u.s. virgin islands")  ,  QLatin1String("VI") },
        { QLatin1String("vietnam")  ,  QLatin1String("VN") },
        { QLatin1String("vanuatu")  ,  QLatin1String("VU") },
        { QLatin1String("wallis and futuna")  ,  QLatin1String("WF") },
        { QLatin1String("samoa")  ,  QLatin1String("WS") },
        { QLatin1String("kosovo")  ,  QLatin1String("XK") },
        { QLatin1String("yemen")  ,  QLatin1String("YE") },
        { QLatin1String("mayotte")  ,  QLatin1String("YT") },
        { QLatin1String("south africa")  ,  QLatin1String("ZA") },
        { QLatin1String("zambia")  ,  QLatin1String("ZM") },
        { QLatin1String("zimbabwe")  ,  QLatin1String("ZW") }};

    static const QSet<QByteArray> acceptedLanguages {
        "af-ZA", "ar", "eu-ES", "bg-BG", "ca-ES", "zh-CN", "zh-TW", "cs-CZ", "da-DK", "nl-BE", "nl-NL", "en-AU", "en-NZ", "en-GB", "en-US", "et-EE", "fi-FI", "fr-CA", "fr-FR", "gl-ES", "de-DE", "el-GR", "hr-HR", "he-IL", "hu-HU", "id-ID", "it-IT", "kk-KZ", "lv-LV", "lt-LT", "ms-MY", "no-NO", "nb-NO", "pl-PL", "pt-BR", "pt-PT", "ro-RO", "ru-RU", "ru-Latn-RU", "ru-Cyrl-RU", "sr-RS", "sk-SK", "sl-SI", "es-ES", "es-419", "sv-SE", "th-TH", "tr-TR", "uk-UA", "vi-VN"
    };
}

QGeoCodingManagerEngineTomTom::QGeoCodingManagerEngineTomTom(const QVariantMap &parameters,
                                                       QGeoServiceProvider::Error *error,
                                                       QString *errorString)
:   QGeoCodingManagerEngine(parameters), m_networkManager(new QNetworkAccessManager(this))
{
    m_language = locale().name().toLatin1();
    if (!acceptedLanguages.contains(m_language))
        m_language = "NGT-Latn";
    m_userAgent = QTomTomCommon::userAgent;
    if (parameters.contains(QStringLiteral("tomtom.useragent")))
        m_userAgent += QLatin1String(" - ") + parameters.value(QStringLiteral("tomtom.useragent")).toString().toLatin1();

    m_accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();

    // ToDo: support locales and "political views"

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}

QGeoCodingManagerEngineTomTom::~QGeoCodingManagerEngineTomTom()
{
}

static QString toString(const QGeoCoordinate &crd)
{
    return QString::number(crd.latitude(), 'f', 10) + ',' + QString::number(crd.longitude(), 'f', 10);
}

static void fillQueryWithBounds(QUrlQuery &query, const QGeoShape &bounds)
{
    if (bounds.isValid()) {
        if (bounds.type() == QGeoShape::CircleType) {
            const QGeoCircle circle(bounds);
            query.addQueryItem(QLatin1String("lat"), QString::number(circle.center().latitude(), 'f', 10));
            query.addQueryItem(QLatin1String("lon"), QString::number(circle.center().longitude(), 'f', 10));
            if (circle.radius())
                query.addQueryItem(QLatin1String("radius"), QString::number(circle.radius(), 'f', 2));
        } else {
            const QGeoRectangle &rect = bounds.boundingGeoRectangle();
            query.addQueryItem(QLatin1String("topLeft"), toString(rect.topLeft()));
            query.addQueryItem(QLatin1String("btmRight"), toString(rect.bottomRight()));
        }
    }
}

QGeoCodeReply *QGeoCodingManagerEngineTomTom::geocode(const QGeoAddress &address, const QGeoShape &bounds)
{
    QUrl url(QTomTomCommon::baseUrlStructuredGeocoding);
    QUrlQuery query;
    query.addQueryItem(QLatin1String("key"), m_accessToken);
    query.addQueryItem(QLatin1String("limit"), QLatin1String("100"));
    fillQueryWithBounds(query, bounds);
    // Fill address
    QString countryCode = address.countryCode();
    if ((countryCode.isEmpty() || countryCode.length() != 2) && !address.country().isEmpty())
        countryCode = countryCodes.value(QLatin1String(address.country().toLower().toLatin1()));

    if (countryCode.isEmpty()) {
        qWarning() << "TomTom structured geocoding REQUIRES a 2 digits country code";
        return nullptr;
    }
    query.addQueryItem(QLatin1String("countryCode"), countryCode);
    query.addQueryItem(QLatin1String("language"), m_language);
    if (!address.street().isEmpty())
        query.addQueryItem(QLatin1String("streetName"), address.street());
    if (!address.city().isEmpty())
        query.addQueryItem(QLatin1String("municipality"), address.city());
    if (!address.district().isEmpty())
        query.addQueryItem(QLatin1String("municipalitySubdivision"), address.district());
    if (!address.postalCode().isEmpty())
        query.addQueryItem(QLatin1String("postalCode"), address.postalCode());
    if (!address.state().isEmpty())
        query.addQueryItem(QLatin1String("countrySubdivision"), address.state());
    if (!address.county().isEmpty())
        query.addQueryItem(QLatin1String("countrySecondarySubdivision"), address.county());
    url.setQuery(query);
    return submitGeocodeRequest(url);
}

QGeoCodeReply *QGeoCodingManagerEngineTomTom::geocode(const QString &address, int limit, int offset, const QGeoShape &bounds)
{  
    const QByteArray encodedSearchString = QUrl::toPercentEncoding(address);
    QUrl url(QTomTomCommon::baseUrlGeocoding + encodedSearchString + QByteArrayLiteral(".json"));
    QUrlQuery query;
    query.addQueryItem(QLatin1String("key"), m_accessToken);
    // ToDo: consider fetching all results in one go (submitting multiple requests), if limit is -1.
    query.addQueryItem(QLatin1String("limit"), QString::number(qBound(1,(limit < 1) ? 100: limit, 100)));
    query.addQueryItem(QLatin1String("ofs"), QString::number(qBound(0,offset,1900)));
    query.addQueryItem(QLatin1String("language"), m_language);
    fillQueryWithBounds(query, bounds);
    url.setQuery(query);

    return submitGeocodeRequest(url);
}

QGeoCodeReply *QGeoCodingManagerEngineTomTom::reverseGeocode(const QGeoCoordinate &coordinate, const QGeoShape &bounds)
{
    QUrl url(QTomTomCommon::baseUrlReverseGeocoding + toString(coordinate) + QByteArrayLiteral(".json"));
    QUrlQuery query;
    query.addQueryItem(QLatin1String("key"), m_accessToken);
    query.addQueryItem(QLatin1String("language"), m_language);
    query.addQueryItem(QLatin1String("returnSpeedLimit"), QLatin1String("true"));
    if (bounds.isValid()) {
        if (bounds.type() == QGeoShape::CircleType) {
            const QGeoCircle circle(bounds);
            if (circle.radius())
                query.addQueryItem(QLatin1String("radius"), QString::number(circle.radius(), 'f', 2));
        } else {
            // This is a simplistic but inaccurate approach.
            // The correct solution would be solving the complementary problem of QGeoCirclePrivate::updateBoundingBox
            const QGeoRectangle &rect = bounds.boundingGeoRectangle();
            const QGeoCoordinate &topLeft = rect.topLeft();
            const QGeoCoordinate &bottomRight = rect.bottomRight();
            const QGeoCoordinate top = QGeoCoordinate(rect.center().longitude(),topLeft.latitude());
            const QGeoCoordinate left = QGeoCoordinate(topLeft.longitude(),rect.center().latitude());
            const QGeoCoordinate bottom = QGeoCoordinate(rect.center().longitude(),bottomRight.latitude());
            qreal radius = rect.center().distanceTo(top);
            radius = qMin(radius, rect.center().distanceTo(left));
            radius = qMin(radius, rect.center().distanceTo(bottom));
            query.addQueryItem(QLatin1String("radius"), QString::number(radius, 'f', 2));
        }
    }

    url.setQuery(query);
    return submitGeocodeRequest(url);
}

QGeoCodeReply *QGeoCodingManagerEngineTomTom::submitGeocodeRequest(const QUrl &url)
{
//    qDebug() << "doGeocoding "<<url;
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    request.setUrl(url);
    QNetworkReply *reply = m_networkManager->get(request);

    QGeoCodeReplyTomTom *geocodeReply = new QGeoCodeReplyTomTom(reply, this);
    if (true) {
        QGeoCodeReplyTomTomPrivate *replyPrivate
                = static_cast<QGeoCodeReplyTomTomPrivate *>(QGeoCodeReplyPrivate::get(*geocodeReply));
        replyPrivate->m_extraData["request_url"] = url;
    }

    connect(geocodeReply, SIGNAL(finished()), this, SLOT(onReplyFinished()));
    connect(geocodeReply, SIGNAL(error(QGeoCodeReply::Error,QString)),
            this, SLOT(onReplyError(QGeoCodeReply::Error,QString)));

    return geocodeReply;
}

void QGeoCodingManagerEngineTomTom::onReplyFinished()
{
    QGeoCodeReply *reply = qobject_cast<QGeoCodeReply *>(sender());
    if (reply)
        emit finished(reply);
}

void QGeoCodingManagerEngineTomTom::onReplyError(QGeoCodeReply::Error errorCode, const QString &errorString)
{
    QGeoCodeReply *reply = qobject_cast<QGeoCodeReply *>(sender());
    if (reply)
        emit error(reply, errorCode, errorString);
}

QT_END_NAMESPACE
