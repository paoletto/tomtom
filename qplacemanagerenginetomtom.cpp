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

#include "qtomtomcommon.h"
#include "qplacemanagerenginetomtom.h"

#include <QtCore/QUrlQuery>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtPositioning/QGeoCircle>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtLocation/private/unsupportedreplies_p.h>
#include <QtLocation/private/qplacesearchrequest_p.h>
#include <QtLocation/qplacesearchrequest.h>
#include <QtLocation/qplaceresult.h>


QPlaceManagerEngineTomTom::QPlaceManagerEngineTomTom(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
    : QPlaceManagerEngine(parameters), m_networkManager(new QNetworkAccessManager(this))
{
    bool ok = true;
    m_userAgent = QTomTomCommon::userAgent;
    if (parameters.contains(QStringLiteral("tomtom.useragent")))
        m_userAgent += QLatin1String(" - ") + parameters.value(QStringLiteral("tomtom.useragent")).toString().toLatin1();

    m_accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString();
    if (m_accessToken.isEmpty()) {
        ok = false;
        *error = QGeoServiceProvider::MissingRequiredParameterError;
        *errorString = QTomTomCommon::missingAccessToken();
    }


    if (ok) {
        *error = QGeoServiceProvider::NoError;
        errorString->clear();
    }

    // ToDo: support languages
    // ToDo: cache categories?
}

QPlaceManagerEngineTomTom::~QPlaceManagerEngineTomTom()
{
}

QPlaceSearchReply *QPlaceManagerEngineTomTom::search(const QPlaceSearchRequest &request)
{
    return qobject_cast<QPlaceSearchReply *>(doPOISearch(request, FullSearch));
}

QPlaceSearchSuggestionReply *QPlaceManagerEngineTomTom::searchSuggestions(const QPlaceSearchRequest &request)
{
    return qobject_cast<QPlaceSearchSuggestionReply *>(doPOISearch(request, SuggestionSearch));
}

QPlaceReply *QPlaceManagerEngineTomTom::initializeCategories()
{
    QNetworkReply *networkReply = nullptr;
    // ToDo: check for connectivity.
    if (m_initStatus == Uninitialized) {
        m_initStatus = Initializing; // ### Qt6: fix this. It is apparently not called only once.
                                    // No point to return a reply at all!
                                    // Now we have to return fake replies instead!

        QUrl requestUrl(QTomTomCommon::baseUrlCategories);
        QUrlQuery queryItems;
        queryItems.addQueryItem(QLatin1String("key"), m_accessToken);
        requestUrl.setQuery(queryItems);
        QNetworkRequest networkRequest(requestUrl);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

        networkReply = m_networkManager->get(networkRequest);
    }
    QPlaceCategoriesInitializationReplyTomTom *reply =
            new QPlaceCategoriesInitializationReplyTomTom(networkReply, this);

    connect(reply, &QPlaceReply::finished, this, &QPlaceManagerEngineTomTom::onReplyFinished);
    connect(reply, QOverload<QPlaceReply::Error, const QString &>::of(&QPlaceReply::error),
            this, &QPlaceManagerEngineTomTom::onReplyError);

    return reply;
}

QString QPlaceManagerEngineTomTom::parentCategoryId(const QString &categoryId) const
{
    if (m_categoryMap.contains(categoryId))
        return m_categoryMap.value(categoryId).parent;
    return QString();
}

QStringList QPlaceManagerEngineTomTom::childCategoryIds(const QString &categoryId) const
{
    if (m_categoryMap.contains(categoryId))
        return m_categoryMap.value(categoryId).childrenWithSynonyms;
    return QStringList();
}

QPlaceCategory QPlaceManagerEngineTomTom::category(const QString &categoryId) const
{
    if (m_categoryMap.contains(categoryId))
        return m_categoryMap.value(categoryId).category;
    return QPlaceCategory();
}

QList<QPlaceCategory> QPlaceManagerEngineTomTom::childCategories(const QString &parentId) const
{
    if (m_categoryMap.contains(parentId))
        return m_categoryMap.value(parentId).childCategories;
    return m_rootCategories;
}

void QPlaceManagerEngineTomTom::onReplyFinished()
{
    QPlaceReply *reply = qobject_cast<QPlaceReply *>(sender());
    if (reply)
        emit finished(reply);
}

void QPlaceManagerEngineTomTom::onReplyError(QPlaceReply::Error errorCode, const QString &errorString)
{
    QPlaceReply *reply = qobject_cast<QPlaceReply *>(sender());
    if (reply)
        emit error(reply, errorCode, errorString);
}

QPlaceReply *QPlaceManagerEngineTomTom::doPOISearch(const QPlaceSearchRequest &request,
                                                    QPlaceManagerEngineTomTom::PlaceSearchType searchType)
{
    int limit = qBound(1, request.limit(), 100);
    if (request.limit() <= 0)
        limit = 100; // Max supported
    const QGeoShape searchArea = request.searchArea();
    const QString searchTerm = request.searchTerm();
    const QString recommendationId = request.recommendationId();
    const QList<QPlaceCategory> placeCategories = request.categories();
    const QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(request);
//    const bool relatedRequest = rpimpl->related;
    const int page = rpimpl->page;

    QByteArray queryString = QUrl::toPercentEncoding(searchTerm);

    QUrl requestUrl(QTomTomCommon::baseUrlPlaces + queryString + QByteArrayLiteral(".json"));

    QUrlQuery queryItems;
    queryItems.addQueryItem(QLatin1String("key"), m_accessToken);
    queryItems.addQueryItem(QLatin1String("limit"), QString::number(limit));
    if (searchType == SuggestionSearch)
        queryItems.addQueryItem(QLatin1String("typeahead"), QLatin1String("true"));
    if (searchArea.isValid() && !searchArea.isEmpty()) {
        const QGeoRectangle &area = searchArea.boundingGeoRectangle();
        queryItems.addQueryItem(QLatin1String("topLeft"), QTomTomCommon::toString(area.topLeft()));
        queryItems.addQueryItem(QLatin1String("btmRight"), QTomTomCommon::toString(area.bottomRight()));
    }
    queryItems.addQueryItem(QLatin1String("ofs"), QString::number(page * limit));
    if (!request.categories().isEmpty()) {
        QSet<QString> cats;
        for (const auto &c: request.categories())
            cats.insert(c.categoryId());
        queryItems.addQueryItem(QLatin1String("categorySet"), cats.values().join(QChar(',')));
    }
    requestUrl.setQuery(queryItems);

    QNetworkRequest networkRequest(requestUrl);
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);

    QNetworkReply *networkReply = m_networkManager->get(networkRequest);
    QPlaceReply *reply;
    if (searchType == FullSearch)
        reply = new QPlaceSearchReplyTomTom(request, networkReply, this);
    else // if (searchType == SuggestionSearch)
        reply = new QPlaceSearchSuggestionReplyTomTom(request, networkReply, this);

    connect(reply, &QPlaceReply::finished, this, &QPlaceManagerEngineTomTom::onReplyFinished);
    connect(reply, QOverload<QPlaceReply::Error, const QString &>::of(&QPlaceReply::error),
            this, &QPlaceManagerEngineTomTom::onReplyError);

    return reply;
}

/* search */

QPlaceSearchReplyTomTom::QPlaceSearchReplyTomTom(const QPlaceSearchRequest &request,
                        QNetworkReply *reply,
                        QPlaceManagerEngineTomTom *parent)
:   QPlaceSearchReply(parent)
{
    Q_ASSERT(parent);
    if (Q_UNLIKELY(!reply)) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }
    setRequest(request);

    connect(reply, &QNetworkReply::finished, this, &QPlaceSearchReplyTomTom::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &QPlaceSearchReplyTomTom::onNetworkError);

    connect(this, &QPlaceReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QPlaceSearchReplyTomTom::~QPlaceSearchReplyTomTom(){}

void QPlaceSearchReplyTomTom::setError(QPlaceReply::Error errorCode, const QString &errorString)
{
    QPlaceReply::setError(errorCode, errorString);
    emit error(errorCode, errorString);
    setFinished(true);
    emit finished();
}

void QPlaceSearchReplyTomTom::onNetworkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    setError(QPlaceReply::CommunicationError, reply->errorString());
}

static QList<QPlaceSearchResult> parseResults(const QJsonObject &responseObject, const QPlaceSearchRequest &request)
{
    const QGeoShape &searchArea = request.searchArea();
    const QVariantList resultsJson = responseObject.value(QLatin1String("results")).toArray().toVariantList();

    QList<QPlaceSearchResult> results;
    for (int i = 0; i < resultsJson.size(); ++i) {
        const QVariantMap resultJson = resultsJson.at(i).toMap();
        const QVariantMap poiData = resultJson.value(QLatin1String("poi")).toMap();
        QPlaceResult pr;
        QPlace place;
        QList<QPlaceCategory> categories;
        QGeoLocation location;
        QGeoAddress address;

        // Parse categories
        for (const auto &c: poiData.value("categories").toList()) {
            QPlaceCategory category;
            category.setName(c.toString());
            categories.append(category);
        }
        place.setCategories(categories);
        // Parse address
        QTomTomCommon::parseAddress(address, resultJson.value(QLatin1String("address")).toMap());
        location.setAddress(address);
        // Parse coordinates
        location.setCoordinate(QTomTomCommon::fromLatLon(resultJson.value(QLatin1String("position")).toMap()));
        // Parse viewport
        QGeoRectangle viewport;
        const QVariantMap viewportJson = resultJson.value(QLatin1String("viewport")).toMap();
        viewport.setTopLeft(QTomTomCommon::fromLatLon(viewportJson.value(QLatin1String("topLeftPoint")).toMap()));
        viewport.setBottomRight(QTomTomCommon::fromLatLon(viewportJson.value(QLatin1String("bottomRightPoint")).toMap()));
        location.setBoundingBox(viewport);
        // Parse entry points
        QVariantList entryPointsJson = resultJson.value(QLatin1String("entryPoints")).toList();
        QVariantList entryPoints;
        for (const auto &e: entryPointsJson) {
            entryPoints.append(QVariant::fromValue(QTomTomCommon::fromLatLon(
                               e.toMap().value(QLatin1String("position")).toMap())));
        }
        location.setExtendedAttributes({{QLatin1String("entryPoints"), entryPoints}});
        place.setLocation(location);
        // Parse name
        place.setName(poiData.value("name").toString());
        // Parse contact details
        if (poiData.contains(QLatin1String("phone"))) {
            QPlaceContactDetail phoneDetail;
            phoneDetail.setLabel(QPlaceContactDetail::Phone);
            phoneDetail.setValue(poiData.value(QLatin1String("phone")).toString());
            place.setContactDetails(QPlaceContactDetail::Phone, { phoneDetail });
        }
        if (poiData.contains(QLatin1String("url"))) {
            QPlaceContactDetail urlDetail;
            urlDetail.setLabel(QPlaceContactDetail::Website);
            urlDetail.setValue(poiData.value(QLatin1String("url")).toString());
            place.setContactDetails(QPlaceContactDetail::Website, { urlDetail });
        }

        // ToDo: check if there are more contact information available to retrieve somehow?
        pr.setPlace(place);
        pr.setTitle(place.name() + QLatin1String(", ") + address.city());
        if (searchArea.isValid())
            pr.setDistance(searchArea.center().distanceTo(location.coordinate()));
        results.append(pr);
    }
    return results;
}

void QPlaceSearchReplyTomTom::onReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (Q_UNLIKELY(reply->error() != QNetworkReply::NoError)) {
        setError(QPlaceReply::CommunicationError, reply->errorString());
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (Q_UNLIKELY(!document.isObject())) {
        setError(ParseError, tr("Response parse error"));
        return;
    }
    const QJsonObject &responseObject = document.object();
    if (Q_UNLIKELY(!responseObject.contains(QLatin1String("results"))
                   || !responseObject.contains(QLatin1String("summary")))) {
        setError(ParseError, tr("Malformed response: missing results or summary"));
        return;
    }

    const QVariantMap summary = responseObject.value(QLatin1String("summary")).toObject().toVariantMap();
    const int totalResults = summary.value(QLatin1String("totalResults")).toInt();
    const int numResults = summary.value(QLatin1String("numResults")).toInt();
    const int offset = summary.value(QLatin1String("offset")).toInt();

    const QPlaceSearchRequest &req = request();
    QList<QPlaceSearchResult> results = parseResults(responseObject, req);
    const QPlaceSearchRequestPrivate *rpimpl = QPlaceSearchRequestPrivate::get(req);

    if (rpimpl->page > 0) {
        QPlaceSearchRequest previous;
        QPlaceSearchRequestPrivate *previousPimpl = QPlaceSearchRequestPrivate::get(previous);
        previousPimpl->page = rpimpl->page - 1;
        previousPimpl->related = true;
        setPreviousPageRequest(previous);
    }

    if (numResults && offset + numResults < totalResults) {
        QPlaceSearchRequest next;
        QPlaceSearchRequestPrivate *nextPimpl = QPlaceSearchRequestPrivate::get(next);
        nextPimpl->page = rpimpl->page + 1;
        nextPimpl->related = true;
        setNextPageRequest(next);
    }

    setResults(results);
    setFinished(true);
    emit finished();
}

/* search suggestions */

QPlaceSearchSuggestionReplyTomTom::QPlaceSearchSuggestionReplyTomTom(const QPlaceSearchRequest &request,
                        QNetworkReply *reply,
                        QPlaceManagerEngineTomTom *parent)
:   QPlaceSearchSuggestionReply(parent), m_request(request)
{
    Q_ASSERT(parent);
    if (Q_UNLIKELY(!reply)) {
        setError(UnknownError, QStringLiteral("Null reply"));
        return;
    }

    connect(reply, &QNetworkReply::finished, this, &QPlaceSearchSuggestionReplyTomTom::onReplyFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
            this, &QPlaceSearchSuggestionReplyTomTom::onNetworkError);

    connect(this, &QPlaceReply::aborted, reply, &QNetworkReply::abort);
    connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
}

QPlaceSearchSuggestionReplyTomTom::~QPlaceSearchSuggestionReplyTomTom(){}

void QPlaceSearchSuggestionReplyTomTom::setError(QPlaceReply::Error errorCode, const QString &errorString)
{
    QPlaceReply::setError(errorCode, errorString);
    emit error(errorCode, errorString);
    setFinished(true);
    emit finished();
}

void QPlaceSearchSuggestionReplyTomTom::onNetworkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    setError(QPlaceReply::CommunicationError, reply->errorString());
}

void QPlaceSearchSuggestionReplyTomTom::onReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (Q_UNLIKELY(reply->error() != QNetworkReply::NoError)) {
        setError(QPlaceReply::CommunicationError, reply->errorString());
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (Q_UNLIKELY(!document.isObject())) {
        setError(ParseError, tr("Response parse error"));
        return;
    }
    const QJsonObject &responseObject = document.object();
    if (Q_UNLIKELY(!responseObject.contains(QLatin1String("results"))
                   || !responseObject.contains(QLatin1String("summary")))) {
        setError(ParseError, tr("Malformed response: missing results or summary"));
        return;
    }

    const QPlaceSearchRequest &req = m_request;
    QList<QPlaceSearchResult> results = parseResults(responseObject, req);
    QStringList suggestions;
    for (const auto &r: results)
        suggestions.append(r.title());

    // ToDo: support pagination here too!

    setSuggestions(suggestions);
    setFinished(true);
    emit finished();
}

/* category initialization */

QPlaceCategoriesInitializationReplyTomTom::QPlaceCategoriesInitializationReplyTomTom(QNetworkReply *reply,
                                  QPlaceManagerEngineTomTom *parent)
:   QPlaceReply(parent)
{
    Q_ASSERT(parent);

    if (reply) {
        connect(reply, &QNetworkReply::finished, this, &QPlaceCategoriesInitializationReplyTomTom::onReplyFinished);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                this, &QPlaceCategoriesInitializationReplyTomTom::onNetworkError);

        connect(this, &QPlaceReply::aborted, reply, &QNetworkReply::abort);
        connect(this, &QObject::destroyed, reply, &QObject::deleteLater);
    } else {
        // Queue a future finished() emission from the reply.
        QMetaObject::invokeMethod(this, "onFakeFinished", Qt::QueuedConnection);
    }
}
QPlaceCategoriesInitializationReplyTomTom::~QPlaceCategoriesInitializationReplyTomTom(){}

void QPlaceCategoriesInitializationReplyTomTom::setError(QPlaceReply::Error errorCode, const QString &errorString)
{
    QPlaceReply::setError(errorCode, errorString);
    emit error(errorCode, errorString);
    setFinished(true);
    emit finished();
}


static void printCategory(const SearchCategoryTomTom&c,
                          const QMap<QString, SearchCategoryTomTom> &categoryMap,
                          int indent = 0)
{
    {
        QDebug deb = qDebug();
        for (int i = 0; i < indent; ++i)
            deb << " *";
        deb << " "<<c.category.name() << "(" << c.id << ")";
    }
    for (const auto &c: c.childrenWithSynonyms) {
        printCategory(categoryMap.value(c),categoryMap,indent + 1);
    }
}

void QPlaceCategoriesInitializationReplyTomTom::onReplyFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    // Parse the categories
    if (Q_UNLIKELY(reply->error() != QNetworkReply::NoError)) {
        setError(QPlaceReply::CommunicationError, reply->errorString());
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (Q_UNLIKELY(!document.isObject())) {
        setError(ParseError, tr("Response parse error"));
        return;
    }
    const QJsonObject &responseObject = document.object();
    if (Q_UNLIKELY(!responseObject.contains(QLatin1String("poiCategories")) )) {
        setError(ParseError, tr("Malformed response: missing poiCategories"));
        return;
    }

    QVariantList categoriesJson = responseObject.value(QLatin1String("poiCategories")).toArray().toVariantList();

    QList<QString> rootCategories;
    QMap<QString, SearchCategoryTomTom> categoryMap;
    QPlaceManagerEngineTomTom *engine = qobject_cast<QPlaceManagerEngineTomTom *>(parent());

    for (const auto &c: categoriesJson) {
        const QVariantMap data = c.toMap();
        SearchCategoryTomTom sc;
        QStringList children;
        const QString id = data.value("id").toString();
        const QString name = data.value("name").toString();
        if (id.isEmpty() || name.isEmpty())
            continue;

        for (const auto &child: data.value("childCategoryIds").toStringList())
            children.append(child);
        sc.children = children;
        sc.childrenWithSynonyms = children;
        sc.category.setName(name);
        sc.category.setCategoryId(id);

        /* forget synonyms for now
        int cnt = 1;
        for (const auto &s: data.value("synonyms").toStringList()) {
            SearchCategoryTomTom sc_;
            sc_.category.setName(s);
            sc_.category.setCategoryId(id);
            const QString id_ = id + QChar('_') + QString::number(cnt); // fake id
            sc_.id = id_;
            sc_.synonymOf = id;
            categoryMap[id_] = sc_;
            ++cnt;
        }
        */

        sc.id = id;
        categoryMap[id] = sc;
    }

    // Rebuild parenting structure.
    for (const QString &key: categoryMap.keys()) {
        if (key.isEmpty())
            continue;
        SearchCategoryTomTom &cat = categoryMap[key];
        for (const QString &childKey: categoryMap.value(key).children) {
            categoryMap[childKey].parent = key;
            cat.childCategories.append(categoryMap[childKey].category);
        }
    }
    // fill childrenWithSynonyms
    for (const QString &key: categoryMap.keys()) {
        if (key.isEmpty())
            continue;
        SearchCategoryTomTom &cat = categoryMap[key];
        if (cat.synonymOf.isEmpty())
            continue;
        cat.parent = categoryMap.value(cat.synonymOf).parent;
        SearchCategoryTomTom &parentCat = categoryMap[cat.parent];
        parentCat.childrenWithSynonyms.append(key);
        parentCat.childCategories.append(cat.category);
    }

    QList<QPlaceCategory> rootCategoryObjects;
    for (const QString &key: categoryMap.keys()) {
        if (key.isEmpty())
            continue;
        if (categoryMap.value(key).parent.isEmpty()) {
            rootCategories.append(key);
            rootCategoryObjects.append(categoryMap.value(key).category);
        }
    }

    // Set the categories into the engine
    engine->m_rootCategoriesIds = rootCategories;
    engine->m_rootCategories = rootCategoryObjects;
    engine->m_categoryMap = categoryMap;

    // ToDo: cache these

    QStringList rcn;
    for (auto c: rootCategories)
        rcn.append(categoryMap.value(c).category.name());

//    for (const auto &c: rootCategories) {
//        printCategory(categoryMap.value(c), categoryMap, 0);
//    }

    qobject_cast<QPlaceManagerEngineTomTom *>(parent())->m_initStatus = QPlaceManagerEngineTomTom::Initialized;
    setFinished(true);
    emit finished();
}

void QPlaceCategoriesInitializationReplyTomTom::onFakeFinished()
{
    // do not touch engine init status
    // qobject_cast<QPlaceManagerEngineTomTom *>(parent())->m_initStatus = QPlaceManagerEngineTomTom::Uninitialized;
    setFinished(true);
    emit finished();
}

void QPlaceCategoriesInitializationReplyTomTom::onNetworkError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    qobject_cast<QPlaceManagerEngineTomTom *>(parent())->m_initStatus = QPlaceManagerEngineTomTom::Uninitialized;
    setError(QPlaceReply::CommunicationError, reply->errorString());
}
