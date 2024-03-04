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

#ifndef QPLACEMANAGERENGINETOMTOM_H
#define QPLACEMANAGERENGINETOMTOM_H

#include <QtLocation/QPlaceManagerEngine>
#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/QPlaceSearchReply>
#include <QtLocation/QPlaceSearchSuggestionReply>
#include <QtNetwork/QNetworkReply>


QT_BEGIN_NAMESPACE

class QNetworkAccessManager;

typedef struct {
    QPlaceCategory category;
    QString id;
    QString synonymOf;
    QString parent;
    QStringList childrenWithSynonyms;
    QStringList children;
    QList<QPlaceCategory> childCategories;
} SearchCategoryTomTom;

class QPlaceManagerEngineTomTom : public QPlaceManagerEngine
{
    Q_OBJECT

public:
    QPlaceManagerEngineTomTom(const QVariantMap &parameters,
                                QGeoServiceProvider::Error *error,
                                QString *errorString);
    ~QPlaceManagerEngineTomTom() override;

    QPlaceSearchReply *search(const QPlaceSearchRequest &request) override;

    QPlaceSearchSuggestionReply *searchSuggestions(const QPlaceSearchRequest &request) override;

    QPlaceReply *initializeCategories() override;
    QString parentCategoryId(const QString &categoryId) const override;
    QStringList childCategoryIds(const QString &categoryId) const override;
    QPlaceCategory category(const QString &categoryId) const override;
    QList<QPlaceCategory> childCategories(const QString &parentId) const override;

//    QList<QLocale> locales() const override;
//    void setLocales(const QList<QLocale> &locales) override;

    // TODO: icon
    //QPlaceIcon icon(const QString &remotePath,
    //                const QList<QPlaceCategory> &categories = QList<QPlaceCategory>()) const;

    //QUrl constructIconUrl(const QPlaceIcon &icon, const QSize &size) const override;

private slots:
    void onReplyFinished();
    void onReplyError(QPlaceReply::Error errorCode, const QString &errorString);

private:
    enum PlaceSearchType {
        FullSearch = 0,
        SuggestionSearch
    };
    enum CategoryInitializationStatus {
        Uninitialized = 0,
        Initializing,
        Initialized
    };

    QPlaceReply *doPOISearch(const QPlaceSearchRequest &request, PlaceSearchType searchType);

    QNetworkAccessManager *m_networkManager;
    QByteArray m_userAgent;
    QString m_accessToken;

//    QList<QLocale> m_locales;
//    QHash<QString, QPlaceCategory> m_categories;
    QStringList m_rootCategoriesIds;
    QList<QPlaceCategory> m_rootCategories;
    QMap<QString, SearchCategoryTomTom> m_categoryMap;
    CategoryInitializationStatus m_initStatus = Uninitialized;

friend class QPlaceCategoriesInitializationReplyTomTom;
};

class QPlaceSearchReplyTomTom : public QPlaceSearchReply
{
    Q_OBJECT

public:
    QPlaceSearchReplyTomTom(const QPlaceSearchRequest &request,
                            QNetworkReply *reply,
                            QPlaceManagerEngineTomTom *parent);
    ~QPlaceSearchReplyTomTom();

private slots:
    void setError(QPlaceReply::Error errorCode, const QString &errorString);
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);
};

class QPlaceSearchSuggestionReplyTomTom : public QPlaceSearchSuggestionReply
{
    Q_OBJECT

public:
    QPlaceSearchSuggestionReplyTomTom(const QPlaceSearchRequest &request,
                                      QNetworkReply *reply,
                                      QPlaceManagerEngineTomTom *parent);
    ~QPlaceSearchSuggestionReplyTomTom();

private slots:
    void setError(QPlaceReply::Error errorCode, const QString &errorString);
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QPlaceSearchRequest m_request;
};

class QPlaceCategoriesInitializationReplyTomTom : public QPlaceReply
{
    Q_OBJECT

public:
    QPlaceCategoriesInitializationReplyTomTom(QNetworkReply *reply,
                                      QPlaceManagerEngineTomTom *parent);
    ~QPlaceCategoriesInitializationReplyTomTom();

private slots:
    void setError(QPlaceReply::Error errorCode, const QString &errorString);
    void onReplyFinished();
    void onFakeFinished();
    void onNetworkError(QNetworkReply::NetworkError error);
};

QT_END_NAMESPACE

#endif // QPLACEMANAGERENGINETOMTOM_H
