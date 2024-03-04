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

#ifndef QGEOTILEDMAPPINGMANAGERENGINETOMTOM_H
#define QGEOTILEDMAPPINGMANAGERENGINETOMTOM_H

#include <QtLocation/QGeoServiceProvider>

#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

QT_BEGIN_NAMESPACE

class QGeoTiledMappingManagerEngineTomTom : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoTiledMappingManagerEngineTomTom(const QVariantMap &parameters,
                                        QGeoServiceProvider::Error *error, QString *errorString);
    ~QGeoTiledMappingManagerEngineTomTom();

    QGeoMap *createMap();

public Q_SLOTS:
    void onCopyrightsFetched(const QByteArray &data);

private:
    QString m_cacheDirectory;
    QImage m_copyrightsImage;
};

QT_END_NAMESPACE

#endif // QGEOTILEDMAPPINGMANAGERENGINETOMTOM_H
