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

#include "qgeoroutingmanagerenginetomtom.h"
#include "qgeoroutereplytomtom.h"
#include "qtomtomcommon.h"
#include <QtLocation/private/qgeorouteparser_p.h>
#include <QtLocation/private/qgeorouteparser_p_p.h>
#include <QtLocation/qgeoroutesegment.h>
#include <QtLocation/private/qgeoroutesegment_p.h>
#include <QtLocation/qgeomaneuver.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QUrlQuery>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static const QSet<QByteArray> acceptedLanguages {
"af-ZA", "ar", "bg-BG", "zh-TW", "cs-CZ", "da-DK", "nl-NL", "en-GB", "en-US", "fi-FI", "fr-FR", "de-DE", "el-GR", "hu-HU", "id-ID", "it-IT", "ko-KR", "lt-LT", "ms-MY", "nb-NO", "pl-PL", "pt-BR", "pt-PT", "ru-RU", "sk-SK", "sl-SI", "es-ES", "es-MX", "sv-SE", "th-TH", "tr-TR"
};

class CompareGeoCoordinate
{
public:
    bool operator()( const QGeoCoordinate &c1, const QGeoCoordinate &c2 ) const
    {
        return c1.latitude() < c2.latitude() ||
                (c1.latitude() == c2.latitude() && c1.longitude() < c2.longitude());
    }
    static bool equal(const QGeoCoordinate &c1, const QGeoCoordinate &c2 )
    {
        return c1.latitude() == c2.latitude() && c1.longitude() == c2.longitude();
    }
};
typedef std::map< QGeoCoordinate, QVector<int>, CompareGeoCoordinate > CoordinateMap;
static QByteArray toString(const QGeoCoordinate &crd)
{
    return QString::number(crd.latitude(), 'f', 10).toLatin1() + ',' + QString::number(crd.longitude(), 'f', 10).toLatin1();
}
class QGeoRouteParserTomTomPrivate;
class QGeoRouteParserTomTom : public QGeoRouteParser
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QGeoRouteParserTomTom)
public:
    QGeoRouteParserTomTom(QGeoRoutingManagerEngineTomTom *parent, const QByteArray &token);
    ~QGeoRouteParserTomTom() override {}
};

class QGeoRouteParserTomTomPrivate :  public QGeoRouteParserPrivate
{
    Q_DECLARE_PUBLIC(QGeoRouteParserTomTom)
public:
    QByteArray m_token;
    QByteArray m_language;
    QGeoRoutingManagerEngineTomTom *m_engine;
    QGeoRouteParserTomTomPrivate(const QByteArray &token, QGeoRoutingManagerEngineTomTom *engine)
        : QGeoRouteParserPrivate(), m_token(token), m_engine(engine)
    {
        m_language = m_engine->locale().name().toLatin1();
        if (!acceptedLanguages.contains(m_language))
            m_language = "en-GB";
    }
    ~QGeoRouteParserTomTomPrivate() override {}
    static const QMap<QGeoRouteRequest::TravelMode, QByteArray> travelModes;
    static QVector<QGeoRouteRequest::TravelMode> travelModesToList(QGeoRouteRequest::TravelModes modes)
    {
        QVector<QGeoRouteRequest::TravelMode> res;
        for (const auto m: travelModes.keys()) {
            if (modes & m)
                res.append(m);
        }

        return res;
    }
    virtual QUrl requestUrl(const QGeoRouteRequest &request, const QString &/*prefix*/) const override
    {
        QByteArray url = QTomTomCommon::baseUrlRouting;
        // Parse waypoints
        QByteArray wpts;
        for (const QGeoCoordinate &w: request.waypoints())
            wpts += ':' + toString(w);
        url += wpts.right(wpts.size() - 1) + QByteArrayLiteral("/json");
        QUrl routingUrl(url);
        QUrlQuery query;
        query.addQueryItem(QLatin1String("key"), m_token);
        query.addQueryItem(QLatin1String("maxAlternatives"), QString::number(request.numberAlternativeRoutes()));
        query.addQueryItem(QLatin1String("travelMode"),
                           travelModes.value(travelModesToList(request.travelModes()).first()));
        query.addQueryItem(QLatin1String("instructionsType"), QLatin1String("text"));
        query.addQueryItem(QLatin1String("language"), m_language);

        // ToDo: support all extras described in
        // - https://developer.tomtom.com/routing-api/routing-api-documentation-routing/common-routing-parameters
        // - https://developer.tomtom.com/routing-api/routing-api-documentation-routing/calculate-route
        // - https://developer.tomtom.com/routing-api/routing-api-documentation/long-distance-ev-routing
        //
        // const QVariantMap &extra = request.extraParameters();
        // Parse extra

        routingUrl.setQuery(query);
        return routingUrl;
    }
    static QGeoCoordinate coordinateFromMap(const QVariant &c) {
        const QVariantMap crd = c.toMap();
        const double lati = crd["latitude"].toDouble();
        const double longi = crd["longitude"].toDouble();
        return QGeoCoordinate(lati, longi);
    }
    virtual QGeoRouteReply::Error parseReply(QList<QGeoRoute> &routes,
                                             QString &errorString,
                                             const QByteArray &reply,
                                             const QGeoRouteRequest &request) const override
    {
        struct LegFinder
        {
            static int leg(const QGeoCoordinate &c, const QVector<CoordinateMap> &coordinateMaps)
            {
                for (int l = 0; l < coordinateMaps.size(); l++) {
                    const CoordinateMap &cm = coordinateMaps.at(l);
                    if (cm.find(c) != cm.end())
                        return l;
                }
                return -1;
            }

            static bool exists(const QGeoCoordinate &c, const QVector<QGeoCoordinate> &legGeometry)
            {
                for (int i = 0; i < legGeometry.size(); ++i)
                    if (CompareGeoCoordinate::equal(legGeometry.at(i), c))
                        return true;
                return false;
            }

            static int findInPath(const QGeoCoordinate &c, const QList<QGeoCoordinate> &routePath, unsigned int initialIdx = 0)
            {
                if (!routePath.size())
                    return -1;
                for (int i = qMin(int(initialIdx), routePath.size() -1); i < routePath.size(); ++i)
                    if (CompareGeoCoordinate::equal(routePath.at(i), c))
                        return i;
                return -1;
            }

            static QList<QGeoCoordinate> popFrontSegment(QList<QGeoCoordinate> &coords, int nextPos)
            {
                QList<QGeoCoordinate> res = coords.mid(0, nextPos + 1);
                coords = coords.mid(nextPos, coords.size() - nextPos);
                return res;
            }
        };

        QJsonDocument document = QJsonDocument::fromJson(reply);
        if (Q_UNLIKELY(!document.isObject())) {
            errorString = QLatin1String("Couldn't parse json.");
            return QGeoRouteReply::ParseError;
        }

        QJsonObject object = document.object();
//        qDebug().noquote()  << document.toJson(QJsonDocument::Indented);
//        QFile file("/tmp/tommy.json");
//        file.open(QIODevice::WriteOnly);
//        file.write(document.toJson(QJsonDocument::Indented));
//        file.close();

        if (Q_UNLIKELY(!object.contains(QLatin1String("routes")))) {
            qWarning() << "No routes in reply!";
            return QGeoRouteReply::UnknownError;
        }

        QVariantList routesJson = object.value(QLatin1String("routes")).toArray().toVariantList();
        for (const QVariant &r: routesJson) {
            const QVariantMap routeMap = r.toMap();

            if (Q_UNLIKELY(!routeMap.contains(QLatin1String("summary")))) {
                qWarning() << "Empty summary!";
                return QGeoRouteReply::UnknownError;
            }
            const QVariantMap summary = routeMap.value(QLatin1String("summary")).toMap();


            if (Q_UNLIKELY(!summary.contains(QLatin1String("travelTimeInSeconds")))) {
                qWarning() << "No travel time!";
                return QGeoRouteReply::UnknownError;
            }
            if (Q_UNLIKELY(!summary.contains(QLatin1String("lengthInMeters")))) {
                qWarning() << "No length!";
                return QGeoRouteReply::UnknownError;
            }
            const int travelTime = summary.value(QLatin1String("travelTimeInSeconds")).toInt();
            const int lengthInMeters = summary.value(QLatin1String("lengthInMeters")).toInt();

            const QVariantMap guidance = routeMap.value(QLatin1String("guidance")).toMap();
            const QVariantList instructionGroups = guidance.value(QLatin1String("instructionGroups")).toList();
            QVariantList instructions = guidance.value("instructions").toList();

            QGeoRoute route;
            route.setTravelTime(travelTime);
            route.setDistance(lengthInMeters);
            route.setTravelMode(travelModesToList(request.travelModes()).first());

            QVector<CoordinateMap> coordinateMaps;
            QList<QList<QGeoCoordinate>> legsGeometry;
            const QVariantList &legs = routeMap.value(QLatin1String("legs")).toList();
            QList<QGeoRouteLeg> routeLegs;
            QList<QGeoCoordinate> routePath;
            int count = 0;
            for (int i = 0; i < legs.size(); ++i) {
                QList<QGeoCoordinate> legGeometry;
                CoordinateMap coordinateMap;
                const QVariantMap l = legs.at(i).toMap();
                const QVariantList geom = l["points"].toList();
                for (int j = 0; j < geom.size(); ++j) {
                    QGeoCoordinate crd(coordinateFromMap(geom.at(j)));
                    if (j == 0 && i == 0) {
                        // Cheating: prepend first instruction coordinate if != first coordinate. Thanks, TomTom..
                        const QGeoCoordinate &firstInstructionPoint = coordinateFromMap(instructions.first().toMap().value("point"));
                        if (!CompareGeoCoordinate::equal(firstInstructionPoint, crd)) {
                            legGeometry.append(firstInstructionPoint);
                            routePath.append(firstInstructionPoint);
                            coordinateMap[firstInstructionPoint].append(count);
                            count += 1;
                        }
                    }

                    legGeometry.append(crd);
                    routePath.append(crd);
                    coordinateMap[crd].append(count);
                    count += 1;
                }

                legsGeometry.append(legGeometry);
                coordinateMaps.append(coordinateMap);
                const QVariantMap summary = l["summary"].toMap();
                const int lengthInMeters = summary["lengthInMeters"].toInt();
                const int travelTimeInSeconds = summary["travelTimeInSeconds"].toInt();
                QGeoRouteLeg routeLeg;
                routeLeg.setPath(legGeometry);
                routeLeg.setLegIndex(i);
                routeLeg.setOverallRoute(route); // QGeoRoute::d_ptr is explicitlySharedDataPointer. Modifiers below won't detach it.
                routeLeg.setDistance(lengthInMeters);
                routeLeg.setTravelTime(travelTimeInSeconds);
                routeLegs << routeLeg;
            }
            // Cheating: append last instruction coordinate if != last coordinate. Thanks, TomTom..
            const QGeoCoordinate &lastInstructionPoint = coordinateFromMap(instructions.last().toMap().value("point"));
            if (!CompareGeoCoordinate::equal(lastInstructionPoint, routePath.last())) {
                legsGeometry.last().append(lastInstructionPoint);
                routePath.append(lastInstructionPoint);
            }

            route.setPath(routePath);

            QVector<int> toRemove;
            QMap<int, int> instructionLeg;

            // The problem: some instructions have a "point" that is not
            // present in the geometry. This is normally when the instruction is of DIRECTION_INFO type.
            // Ideal solution: figure out where this point lies, within
            // the above polyline, and inject it.
            // Temporary solution: Drop the instruction.
            for (int i = 0; i < instructions.size() - 1; ++i) { // do not remove the last instruction. it might not match the last position.
                const QGeoCoordinate ic = coordinateFromMap(instructions.at(i).toMap().value("point"));
                bool found = LegFinder::leg(ic, coordinateMaps) >= 0;
                if (found)
                    continue;
                // CLearly there are more cases like these
//                Q_ASSERT(instructions.at(i).toMap().value("instructionType").toString() == QLatin1String("DIRECTION_INFO")
//                      || instructions.at(i).toMap().value("maneuver").toString() == QLatin1String("TRY_MAKE_UTURN"));
                toRemove.append(i);
            }

            for (int i = toRemove.size() - 1; i >= 0; i--) {
                instructions.removeAt(toRemove.at(i));
            }
            if (Q_UNLIKELY(instructions.size() < 2)) {
                qWarning() << "Not enough instructions";
                errorString = QLatin1String("Not enough instructions");
                return QGeoRouteReply::ParseError;
            }

            // validate sequentiality
            int cur = 0;
            for (int i = 1; i < instructions.size(); ++i) {
                const QGeoCoordinate ic0 = coordinateFromMap(instructions.at(i-1).toMap().value("point"));
                const QGeoCoordinate ic1 = coordinateFromMap(instructions.at(i).toMap().value("point"));
                const int ic0idx = LegFinder::findInPath(ic0, route.path(), cur);
                const int ic1idx = LegFinder::findInPath(ic1, route.path(), cur);
                if (Q_UNLIKELY( ic0idx > ic1idx )) {
                    qWarning() << "Instructions out of geographical order "<<ic0idx<< " " << ic1idx;
                    qWarning() << "Next ic1idx: " << LegFinder::findInPath(ic1, route.path(), ic0idx + 1);
                    errorString = QLatin1String("Instructions out of geographical order");
                    return QGeoRouteReply::ParseError;
                }
                cur = ic1idx;
            }

            QVector<QVector<QGeoRouteSegment>> legSegments;
            legSegments << QVector<QGeoRouteSegment>();
            QGeoRouteSegment segment;
            QGeoRouteSegment lastSegment;
            int currentLeg = 0;
            int totalTime = 0;
            int totalDistance = 0;
            int legCursor = 0;
            QList<QGeoCoordinate> legGeometry = legsGeometry.first();

            for (int idx = 0; idx < instructions.size(); ++idx) {
                lastSegment = segment;
                segment = QGeoRouteSegment();
                QVariantMap i = instructions.at(idx).toMap();
                QGeoCoordinate c = coordinateFromMap(i["point"]);
                bool switchLeg = false;
                bool legHeadInjected = false;

                if (idx == 0) {
                    const QGeoCoordinate crd = legsGeometry.first().first();
                    Q_ASSERT(CompareGeoCoordinate::equal(c, legsGeometry.first().first()));
                    Q_ASSERT(i["instructionType"] == QLatin1String("LOCATION_DEPARTURE"));
                } else if (legCursor == 0) { // Beginning of leg
                    // Cheating: Verify that the first instruction matches the coordinate.
                    // If not, inject a "Depart" instruction.
                    if (!CompareGeoCoordinate::equal(c, legGeometry.first())) {
                        legHeadInjected = true;
                        c = legGeometry.first();
                        i = {
                            {"maneuver", "DEPART"},
                            {"routeOffsetInMeters", instructions.at(idx-1).toMap().value("routeOffsetInMeters")}, // Use last (waypoint reached)
                            {"travelTimeInSeconds", instructions.at(idx-1).toMap().value("travelTimeInSeconds")}, // Use last (waypoint reached)
                            {"message", QGeoRouteParserTomTom::tr("Depart from waypoint")}
                        };
                    }
                }

                int nextId = legGeometry.size() -1;
                // Find next
                if (idx < instructions.size() - 1) {
                    int nextCur = (legHeadInjected) ? idx : idx+1;
                    const QGeoCoordinate &nextPos = coordinateFromMap(instructions.at(nextCur).toMap()["point"]);
                    // For simplicity, let's assume one leg can't have less than 2 instructions.
                    nextId = LegFinder::findInPath(nextPos, legGeometry, legCursor);
                    if (nextId < 0) { // switch
                        switchLeg = true;
                        nextId = legGeometry.size() -1;
                    }
                } else // last instruction, go to end
                    nextId = legGeometry.size() -1;

                // Extract geometry from legGeometry
                QList<QGeoCoordinate> segmentGeometry = LegFinder::popFrontSegment(legGeometry, nextId);

                const int routeOffset = i["routeOffsetInMeters"].toInt();
                const int travelTimeInSeconds = i["travelTimeInSeconds"].toInt();
                segment.setDistance(routeOffset  -  totalDistance); // This is incorrect. this is the offset from the start. fixed below.
                segment.setTravelTime(travelTimeInSeconds - totalTime); // This is incorrect too. it is related to the previous seg. fixed below.
                totalTime = travelTimeInSeconds;
                totalDistance = routeOffset;
                QGeoManeuver maneuver;
                maneuver.setInstructionText(i["message"].toString());
                maneuver.setPosition(c);
                maneuver.setWaypoint(c);
                maneuver.setDistanceToNextInstruction(segment.distance());
                maneuver.setTimeToNextInstruction(segment.travelTime());
                maneuver.setDirection(maneuverDirections.value(i["maneuver"].toString().toLatin1(),
                                                                QGeoManeuver::NoDirection));
                // use  <drivingSide> to flip the U-turn side, if necessary.
                const QByteArray drivingSide = i["drivingSide"].toString().toLatin1();
                if (maneuver.direction() == QGeoManeuver::DirectionUTurnLeft
                        && drivingSide == QByteArrayLiteral("LEFT"))
                    maneuver.setDirection(QGeoManeuver::DirectionUTurnRight);

//                maneuver.setExtendedAttributes(i);
                segment.setManeuver(maneuver);
                segment.setPath(segmentGeometry);

                if (idx > 0)
                    lastSegment.setNextRouteSegment(segment);

                if (switchLeg || idx == instructions.size() - 1) {
                    // Note: this is needed in order to terminate the leg!
                    QGeoRouteSegmentPrivate *segmentPrivate = QGeoRouteSegmentPrivate::get(segment);
                    segmentPrivate->setLegLastSegment(true);
                }
                legSegments.last().append(segment);

                if (switchLeg) {
                    currentLeg++;
                    legGeometry = legsGeometry.at(currentLeg);
                    legSegments << QVector<QGeoRouteSegment>();
                }

                if (legHeadInjected)
                    idx--;
            }

            // Then split segments into legs
            for (int legIndex = 0; legIndex < routeLegs.size(); ++legIndex) {
                QGeoRouteLeg &routeLeg = const_cast<QGeoRouteLeg &>(routeLegs.at(legIndex)); // do not detach
                QVector<QGeoRouteSegment> segments = legSegments.at(legIndex);
                routeLeg.setFirstRouteSegment(segments.first());
                // and fix the distances and traveltimes, specified for maneuvers at the END of a segment, not at the beginning.
                for (int segmentIndex = 0; segmentIndex < segments.size(); segmentIndex++) {
                    QGeoRouteSegment &current = segments[segmentIndex];
                    if (segmentIndex == segments.size() - 1) {
                        current.setTravelTime(0);
                        current.setDistance(0);
                        QGeoManeuver currentManeuver = current.maneuver();
                        currentManeuver.setTimeToNextInstruction(0);
                        currentManeuver.setDistanceToNextInstruction(0);
                        current.setManeuver(currentManeuver);
                    } else {
                        QGeoRouteSegment &next = segments[segmentIndex + 1];
                        current.setTravelTime(next.travelTime());
                        current.setDistance(next.distance());
                        QGeoManeuver currentManeuver = current.maneuver();
                        currentManeuver.setTimeToNextInstruction(next.maneuver().timeToNextInstruction());
                        currentManeuver.setDistanceToNextInstruction(next.maneuver().distanceToNextInstruction());
                        current.setManeuver(currentManeuver);
                    }
                }
            }
            route.setFirstRouteSegment(legSegments.first().first());
            route.setRouteLegs(routeLegs);
            routes.append(route);
        }

        // setError(QGeoRouteReply::NoError, status);  // can't do this, or NoError is emitted and does damages
        return QGeoRouteReply::NoError;
    }
    static const QMap<QByteArray, QGeoManeuver::InstructionDirection> maneuverDirections;
};

const QMap<QGeoRouteRequest::TravelMode, QByteArray> QGeoRouteParserTomTomPrivate::travelModes {{
        { QGeoRouteRequest::CarTravel,          QByteArrayLiteral("car")   },
        { QGeoRouteRequest::PedestrianTravel,   QByteArrayLiteral("pedestrian")   },
        { QGeoRouteRequest::BicycleTravel,      QByteArrayLiteral("bicycle")   },
        { QGeoRouteRequest::PublicTransitTravel,QByteArrayLiteral("bus")   },
        { QGeoRouteRequest::TruckTravel,        QByteArrayLiteral("truck")   }
}};

const QMap<QByteArray, QGeoManeuver::InstructionDirection> QGeoRouteParserTomTomPrivate::maneuverDirections {{
        {"ARRIVE" ,                 QGeoManeuver::NoDirection }, // You have arrived.
        {"ARRIVE_LEFT" ,            QGeoManeuver::NoDirection }, // You have arrived. Your destination is on the left.
        {"ARRIVE_RIGHT" ,           QGeoManeuver::NoDirection }, // You have arrived. Your destination is on the right.
        {"DEPART" ,                 QGeoManeuver::NoDirection }, // Leave.
        {"STRAIGHT" ,               QGeoManeuver::DirectionForward }, // Keep straight on.
        {"KEEP_RIGHT" ,             QGeoManeuver::DirectionLightRight }, // Keep right.
        {"BEAR_RIGHT" ,             QGeoManeuver::DirectionBearRight }, // Bear right.
        {"TURN_RIGHT" ,             QGeoManeuver::DirectionRight }, // Turn right.
        {"SHARP_RIGHT" ,            QGeoManeuver::DirectionHardRight }, // Turn sharp right.
        {"KEEP_LEFT" ,              QGeoManeuver::DirectionLightLeft }, // Keep left.
        {"BEAR_LEFT" ,              QGeoManeuver::DirectionBearLeft }, // Bear left.
        {"TURN_LEFT" ,              QGeoManeuver::DirectionLeft }, // Turn left.
        {"SHARP_LEFT" ,             QGeoManeuver::DirectionHardLeft }, // Turn sharp left.
        {"MAKE_UTURN" ,             QGeoManeuver::DirectionUTurnLeft }, // Make a U-turn.
        {"ENTER_MOTORWAY" ,         QGeoManeuver::NoDirection }, // Take the motorway.
        {"ENTER_FREEWAY" ,          QGeoManeuver::NoDirection }, // Take the freeway.
        {"ENTER_HIGHWAY" ,          QGeoManeuver::NoDirection }, // Take the highway.
        {"TAKE_EXIT" ,              QGeoManeuver::NoDirection }, // Take the exit.
        {"MOTORWAY_EXIT_LEFT" ,     QGeoManeuver::DirectionLeft }, // Take the left exit.
        {"MOTORWAY_EXIT_RIGHT" ,    QGeoManeuver::DirectionRight }, // Take the right exit.
        {"TAKE_FERRY" ,             QGeoManeuver::NoDirection }, // Take the ferry.
        {"ROUNDABOUT_CROSS" ,       QGeoManeuver::DirectionForward }, // Cross the roundabout.
        {"ROUNDABOUT_RIGHT" ,       QGeoManeuver::DirectionRight }, // At the roundabout take the exit on the right.
        {"ROUNDABOUT_LEFT" ,        QGeoManeuver::DirectionLeft }, // At the roundabout take the exit on the left.
        {"ROUNDABOUT_BACK" ,        QGeoManeuver::DirectionUTurnLeft }, // Go around the roundabout.
        {"TRY_MAKE_UTURN" ,         QGeoManeuver::DirectionUTurnLeft }, // Try to make a U-turn.
        {"FOLLOW" ,                 QGeoManeuver::DirectionForward }, // Follow.
        {"SWITCH_PARALLEL_ROAD" ,   QGeoManeuver::NoDirection }, // Switch to the parallel road.
        {"SWITCH_MAIN_ROAD" ,       QGeoManeuver::NoDirection }, // Switch to the main road.
        {"ENTRANCE_RAMP" ,          QGeoManeuver::NoDirection }, // Take the ramp.
        {"WAYPOINT_LEFT" ,          QGeoManeuver::NoDirection }, // You have reached the waypoint. It is on the left.
        {"WAYPOINT_RIGHT" ,         QGeoManeuver::NoDirection }, // You have reached the waypoint. It is on the right.
        {"WAYPOINT_REACHED" ,       QGeoManeuver::NoDirection } // You have reached the waypoint.
}};

QGeoRouteParserTomTom::QGeoRouteParserTomTom(QGeoRoutingManagerEngineTomTom *parent, const QByteArray &token)
    : QGeoRouteParser(*new QGeoRouteParserTomTomPrivate(token, parent), parent)
{
}

QGeoRoutingManagerEngineTomTom::QGeoRoutingManagerEngineTomTom(const QVariantMap &parameters,
                                                         QGeoServiceProvider::Error *error,
                                                         QString *errorString)
    : QGeoRoutingManagerEngine(parameters),
      m_networkManager(new QNetworkAccessManager(this))
{
    m_userAgent = QTomTomCommon::userAgent;
    if (parameters.contains(QStringLiteral("tomtom.useragent")))
        m_userAgent += QLatin1String(" - ") + parameters.value(QStringLiteral("tomtom.useragent")).toString().toLatin1();

    const QByteArray accessToken = parameters.value(QStringLiteral("tomtom.access_token")).toString().toLatin1();
    m_includeJson = parameters.value(QStringLiteral("tomtom.routing.include_json")).toBool();
    m_debugQuery = parameters.value(QStringLiteral("tomtom.routing.debug_query")).toBool();

    QGeoRouteParserTomTom *parser = new QGeoRouteParserTomTom(this, accessToken);
    m_routeParser = parser;

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}

QGeoRoutingManagerEngineTomTom::~QGeoRoutingManagerEngineTomTom()
{
}

QGeoRouteReply* QGeoRoutingManagerEngineTomTom::calculateRoute(const QGeoRouteRequest &request)
{
    const QList<QGeoCoordinate> &waypoints = request.waypoints();
    if (Q_UNLIKELY(!waypoints.length())) {
        qWarning() << "At least 2 waypoints are required";
        return nullptr;
    }
    QNetworkRequest req;
    req.setHeader(QNetworkRequest::UserAgentHeader, m_userAgent);
    req.setUrl(routeParser()->requestUrl(request, QString()));
    qDebug() << "QGeoRoutingManagerEngineTomTom::calculateRoute "<< req.url();
    QNetworkReply *reply = m_networkManager->get(req);
    QGeoRouteReplyTomTom *routeReply = new QGeoRouteReplyTomTom(reply, request, this);
    connect(routeReply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(routeReply, SIGNAL(error(QGeoRouteReply::Error,QString)),
            this, SLOT(replyError(QGeoRouteReply::Error,QString)));
    return routeReply;
}

const QGeoRouteParser *QGeoRoutingManagerEngineTomTom::routeParser() const
{
    return m_routeParser;
}

void QGeoRoutingManagerEngineTomTom::replyFinished()
{
    QGeoRouteReply *reply = qobject_cast<QGeoRouteReply *>(sender());
    if (reply)
        emit finished(reply);
}

void QGeoRoutingManagerEngineTomTom::replyError(QGeoRouteReply::Error errorCode,
                                             const QString &errorString)
{
    QGeoRouteReply *reply = qobject_cast<QGeoRouteReply *>(sender());
    if (reply)
        emit error(reply, errorCode, errorString);
}

#include "qgeoroutingmanagerenginetomtom.moc"

QT_END_NAMESPACE
