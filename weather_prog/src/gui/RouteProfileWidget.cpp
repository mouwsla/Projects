#include "aviwx/gui/RouteProfileWidget.h"

#include "aviwx/Geo.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace aviwx::gui {
namespace {

constexpr int kLeft = 54;
constexpr int kRight = 22;
constexpr int kTop = 26;
constexpr int kBottom = 34;

QString qstr(const std::string& value)
{
    return QString::fromStdString(value);
}

QString hazardTypeRu(HazardType type)
{
    switch (type) {
    case HazardType::Thunderstorm:
        return "гроза / CB";
    case HazardType::Icing:
        return "обледенение";
    case HazardType::Turbulence:
        return "турбулентность";
    case HazardType::LowVisibility:
        return "низкая видимость";
    case HazardType::LowCeiling:
        return "низкая облачность";
    case HazardType::WindShear:
        return "сдвиг ветра";
    case HazardType::VolcanicAsh:
        return "пепел";
    case HazardType::HeavyPrecipitation:
        return "осадки";
    case HazardType::Unknown:
        return "явление";
    }
    return "явление";
}

bool timeOverlaps(double a0, double a1, double b0, double b1)
{
    return std::max(a0, b0) <= std::min(a1, b1);
}

bool altitudeOverlaps(int a0, int a1, int b0, int b1)
{
    return std::max(a0, b0) <= std::min(a1, b1);
}

} // namespace

RouteProfileWidget::RouteProfileWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(190);
    setMaximumHeight(260);
}

void RouteProfileWidget::setScenario(const Scenario* scenario)
{
    scenario_ = scenario;
    update();
}

void RouteProfileWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#f7f9fc"));

    if (!scenario_ || scenario_->route.waypoints.size() < 2) {
        drawEmpty(painter);
        return;
    }

    drawProfile(painter);
}

void RouteProfileWidget::drawEmpty(QPainter& painter)
{
    painter.setPen(QColor("#687386"));
    painter.drawText(rect(), Qt::AlignCenter, "Профиль маршрута");
}

void RouteProfileWidget::drawProfile(QPainter& painter)
{
    const auto& waypoints = scenario_->route.waypoints;
    const QRectF plot = rect().adjusted(kLeft, kTop, -kRight, -kBottom);

    double max_eta = 1.0;
    int max_altitude = 1000;
    for (const Waypoint& waypoint : waypoints) {
        max_eta = std::max(max_eta, waypoint.eta_min);
        max_altitude = std::max(max_altitude, waypoint.altitude_ft);
    }
    for (const WeatherHazard& hazard : scenario_->hazards) {
        max_altitude = std::max(max_altitude, std::min(hazard.top_ft, 45000));
    }
    max_altitude = static_cast<int>(std::ceil(max_altitude / 1000.0) * 1000.0);

    auto xForTime = [&](double eta_min) {
        return plot.left() + std::clamp(eta_min / max_eta, 0.0, 1.0) * plot.width();
    };
    auto yForAltitude = [&](double altitude_ft) {
        return plot.bottom() - std::clamp(altitude_ft / max_altitude, 0.0, 1.0) * plot.height();
    };

    painter.setPen(QPen(QColor("#d4dbe7"), 1));
    painter.setBrush(QColor("#ffffff"));
    painter.drawRoundedRect(plot, 8, 8);

    QFont label_font = painter.font();
    label_font.setPointSize(9);
    painter.setFont(label_font);

    painter.setPen(QPen(QColor("#d9e0ea"), 1, Qt::DashLine));
    for (int i = 0; i <= 4; ++i) {
        const double y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor("#687386"));
        painter.drawText(QRectF(4, y - 9, kLeft - 10, 18), Qt::AlignRight | Qt::AlignVCenter,
            QString::number(static_cast<int>(max_altitude * i / 4.0)));
        painter.setPen(QPen(QColor("#d9e0ea"), 1, Qt::DashLine));
    }

    for (const WeatherHazard& hazard : scenario_->hazards) {
        QColor fill = hazardColor(hazard.type);
        fill.setAlpha(72);
        QColor stroke = hazardColor(hazard.type).darker(125);
        stroke.setAlpha(170);

        for (std::size_t i = 0; i + 1 < waypoints.size(); ++i) {
            const Waypoint& a = waypoints[i];
            const Waypoint& b = waypoints[i + 1];
            const double t0 = std::min(a.eta_min, b.eta_min);
            const double t1 = std::max(a.eta_min, b.eta_min);
            const int alt0 = std::min(a.altitude_ft, b.altitude_ft);
            const int alt1 = std::max(a.altitude_ft, b.altitude_ft);

            if (!timeOverlaps(t0, t1, hazard.valid_from_min, hazard.valid_to_min)
                || !altitudeOverlaps(alt0, alt1, hazard.base_ft, hazard.top_ft)
                || hazard.polygon.size() < 3) {
                continue;
            }

            const double distance_nm = segment_polygon_distance_nm(a.position, b.position, hazard.polygon);
            if (distance_nm > scenario_->route_corridor_nm) {
                continue;
            }

            const double left = xForTime(std::max(t0, hazard.valid_from_min));
            const double right = xForTime(std::min(t1, hazard.valid_to_min));
            const double top = yForAltitude(std::min(hazard.top_ft, max_altitude));
            const double bottom = yForAltitude(std::max(0, hazard.base_ft));
            QRectF band(QPointF(left, top), QPointF(right, bottom));
            band = band.normalized();
            if (band.width() < 8) {
                band.setWidth(8);
            }
            if (band.height() < 16) {
                band.setHeight(16);
            }

            painter.setPen(QPen(stroke, 1.2));
            painter.setBrush(fill);
            painter.drawRoundedRect(band, 4, 4);

            const QString label = hazardTypeRu(hazard.type);
            if (band.width() > 58 && band.height() > 18) {
                painter.setPen(QColor("#1f2733"));
                painter.drawText(band.adjusted(4, 0, -4, 0), Qt::AlignCenter, label);
            }
        }
    }

    QPainterPath route_path;
    route_path.moveTo(xForTime(waypoints.front().eta_min), yForAltitude(waypoints.front().altitude_ft));
    for (std::size_t i = 1; i < waypoints.size(); ++i) {
        route_path.lineTo(xForTime(waypoints[i].eta_min), yForAltitude(waypoints[i].altitude_ft));
    }

    painter.setPen(QPen(QColor("#164f8b"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(route_path);

    painter.setBrush(QColor("#ffffff"));
    painter.setPen(QPen(QColor("#164f8b"), 2));
    for (const Waypoint& waypoint : waypoints) {
        const QPointF point(xForTime(waypoint.eta_min), yForAltitude(waypoint.altitude_ft));
        painter.drawEllipse(point, 4.5, 4.5);
        painter.setPen(QColor("#1f2733"));
        painter.drawText(QRectF(point.x() - 42, plot.bottom() + 7, 84, 18), Qt::AlignCenter, qstr(waypoint.name));
        painter.setPen(QPen(QColor("#164f8b"), 2));
    }

    QFont title_font = painter.font();
    title_font.setBold(true);
    painter.setFont(title_font);
    painter.setPen(QColor("#354153"));
    painter.drawText(QRectF(plot.left(), 4, plot.width(), 20), Qt::AlignLeft | Qt::AlignVCenter,
        "Профиль маршрута: ETA и высота");
    painter.setFont(label_font);
    painter.setPen(QColor("#687386"));
    painter.drawText(QRectF(4, plot.top() - 2, kLeft - 10, 18), Qt::AlignRight | Qt::AlignVCenter, "ft");
    painter.drawText(QRectF(plot.right() - 110, 4, 110, 20), Qt::AlignRight | Qt::AlignVCenter,
        QString("ETA %1 min").arg(max_eta, 0, 'f', 0));
}

QColor RouteProfileWidget::hazardColor(HazardType type) const
{
    switch (type) {
    case HazardType::Thunderstorm:
        return QColor("#d43d3d");
    case HazardType::Icing:
        return QColor("#1c9ed6");
    case HazardType::Turbulence:
        return QColor("#d78a19");
    case HazardType::LowVisibility:
        return QColor("#4a6fa5");
    case HazardType::LowCeiling:
        return QColor("#5c7c4f");
    case HazardType::WindShear:
        return QColor("#a64ac9");
    case HazardType::VolcanicAsh:
        return QColor("#6d7278");
    case HazardType::HeavyPrecipitation:
        return QColor("#218c74");
    case HazardType::Unknown:
        return QColor("#596170");
    }
    return QColor("#596170");
}

} // namespace aviwx::gui
