#include "aviwx/gui/WeatherMapWidget.h"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLineF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QResizeEvent>
#include <QTimer>
#include <QToolTip>
#include <QTransform>
#include <QWheelEvent>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <utility>

namespace aviwx::gui {
namespace {

constexpr int kMargin = 34;
constexpr int kCacheMargin = 220;

const QColor kBackground{"#0b1118"};
const QColor kFrame{"#27384a"};
const QColor kGridLabel{"#6f8195"};

QString qstr(const std::string& value)
{
    return QString::fromStdString(value);
}

QString hazardTypeRu(HazardType type)
{
    switch (type) {
    case HazardType::Thunderstorm:      return "конвекция / CAPE";
    case HazardType::Icing:             return "0°C + влага";
    case HazardType::Turbulence:        return "нет данных Open-Meteo";
    case HazardType::LowVisibility:     return "низкая видимость";
    case HazardType::LowCeiling:        return "низкая облачность";
    case HazardType::WindShear:         return "нет данных Open-Meteo";
    case HazardType::VolcanicAsh:       return "нет данных Open-Meteo";
    case HazardType::HeavyPrecipitation:return "осадки";
    case HazardType::Unknown:           return "неизвестное явление";
    }
    return "неизвестное явление";
}

QString severityRu(Severity severity)
{
    switch (severity) {
    case Severity::Light:    return "слабая";
    case Severity::Moderate: return "умеренная";
    case Severity::Severe:   return "сильная";
    case Severity::Extreme:  return "экстремальная";
    }
    return "неизвестная";
}

QString riskLevelRu(RiskLevel level)
{
    switch (level) {
    case RiskLevel::Low:     return "низкий";
    case RiskLevel::Medium:  return "умеренный";
    case RiskLevel::High:    return "высокий";
    case RiskLevel::Extreme: return "экстремальный";
    }
    return "нет данных";
}

double niceGridStep(double range)
{
    if (range <= 0.5) return 0.1;
    if (range <= 1.0) return 0.2;
    if (range <= 2.5) return 0.5;
    if (range <= 6.0) return 1.0;
    return 2.0;
}

QPainterPath pathFromPolygon(const QPolygonF& polygon)
{
    QPainterPath path;
    if (polygon.empty()) {
        return path;
    }
    path.moveTo(polygon.front());
    for (qsizetype i = 1; i < polygon.size(); ++i) {
        path.lineTo(polygon[i]);
    }
    path.closeSubpath();
    return path;
}

QString hazardTooltip(const WeatherHazard& hazard)
{
    return QString("%1\n%2, %3\n%4-%5 ft\nДействует: %6-%7 мин")
        .arg(qstr(hazard.id))
        .arg(hazardTypeRu(hazard.type))
        .arg(severityRu(hazard.severity))
        .arg(hazard.base_ft)
        .arg(hazard.top_ft)
        .arg(hazard.valid_from_min, 0, 'f', 0)
        .arg(hazard.valid_to_min, 0, 'f', 0);
}

std::string lower_copy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

struct SxfLayerStyle {
    QColor line;
    QColor fill;
    double width{0.7};
    Qt::PenStyle pen_style{Qt::SolidLine};
    bool fill_polygons{true};
};

SxfLayerStyle styleForSxfLayer(const std::string& name)
{
    const std::string layer = lower_copy(name);
    auto has = [&layer](const char* token) {
        return layer.find(token) != std::string::npos;
    };

    if (has("water") || has("hydro") || has("river") || has("lake") || has("ocean")) {
        return {QColor(110, 168, 208, 180), QColor(40, 96, 134, 120), 1.0, Qt::SolidLine, true};
    }
    if (has("vegetation") || has("forest") || has("wood")) {
        return {QColor(64, 110, 78, 60), QColor(44, 84, 58, 96), 0.6, Qt::SolidLine, true};
    }
    if (has("populated") || has("city") || has("settlement") || has("building") || has("urban")) {
        return {QColor(152, 161, 176, 135), QColor(96, 105, 120, 86), 0.8, Qt::SolidLine, true};
    }
    if (has("highway") || has("motorway") || has("road")) {
        return {QColor(228, 186, 118, 170), QColor(0, 0, 0, 0), 1.2, Qt::SolidLine, false};
    }
    if (has("railway") || has("railroad")) {
        return {QColor(178, 188, 202, 150), QColor(0, 0, 0, 0), 0.9, Qt::DashLine, false};
    }
    if (has("boundary") || has("border") || has("admin")) {
        return {QColor(170, 148, 216, 140), QColor(0, 0, 0, 0), 0.9, Qt::DashLine, false};
    }
    if (has("airport") || has("airfield") || has("aero")) {
        return {QColor(255, 211, 78, 195), QColor(122, 98, 32, 80), 1.1, Qt::SolidLine, true};
    }
    if (has("relief") || has("contour") || has("elevation")) {
        return {QColor(98, 112, 102, 52), QColor(0, 0, 0, 0), 0.55, Qt::SolidLine, false};
    }
    // minor / unclassified geometry — kept low-contrast so the map stays readable
    return {QColor(108, 126, 144, 80), QColor(70, 88, 106, 30), 0.6, Qt::SolidLine, true};
}

} // namespace

WeatherMapWidget::WeatherMapWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(640, 520);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    visible_hazards_ = {
        HazardType::Thunderstorm, HazardType::Icing, HazardType::Turbulence,
        HazardType::WindShear, HazardType::VolcanicAsh, HazardType::HeavyPrecipitation,
        HazardType::LowVisibility, HazardType::LowCeiling, HazardType::Unknown,
    };
    for (HazardType type : {
             HazardType::Thunderstorm, HazardType::Icing, HazardType::Turbulence,
             HazardType::LowVisibility, HazardType::LowCeiling, HazardType::WindShear,
             HazardType::VolcanicAsh, HazardType::HeavyPrecipitation, HazardType::Unknown}) {
        layer_alpha_[type] = 1.0;
    }

    clock_.start();
    last_tick_ms_ = clock_.elapsed();
    anim_timer_ = new QTimer(this);
    anim_timer_->setInterval(16);
    connect(anim_timer_, &QTimer::timeout, this, [this] { onAnimationTick(); });
    anim_timer_->start();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void WeatherMapWidget::setScenario(const Scenario* scenario, const RiskReport* report)
{
    const bool changed = scenario_ != scenario;
    scenario_ = scenario;
    report_ = report;
    selected_hazard_id_.clear();
    if (changed) {
        resetView();
    } else {
        update();
    }
}

void WeatherMapWidget::setBaseMap(std::shared_ptr<const GisMap> map, bool reset_view)
{
    const bool map_changed = base_map_.get() != map.get();
    base_map_ = std::move(map);
    if (map_changed) {
        recomputeDataBounds();
        invalidateBaseCache();
        if (base_map_ && !base_map_->empty()) {
            map_fade_ = 0.0; // fade the freshly loaded SXF sheets in
        }
    }
    if (reset_view) {
        resetView();
    } else {
        update();
    }
}

void WeatherMapWidget::setLiveWeather(std::vector<WeatherHazard> hazards, std::vector<LiveWeatherSample> samples)
{
    live_hazards_ = std::move(hazards);
    live_samples_ = std::move(samples);
    selected_hazard_id_.clear();
    recomputeDataBounds();
    weather_fade_ = 0.0;
    update();
}

void WeatherMapWidget::setHazardVisible(HazardType type, bool visible)
{
    if (visible) {
        visible_hazards_.insert(type);
    } else {
        visible_hazards_.erase(type);
    }
    update();
}

void WeatherMapWidget::setWeatherMode(WeatherMode mode)
{
    if (mode == weather_mode_) {
        return;
    }
    prev_weather_mode_ = weather_mode_;
    weather_mode_ = mode;
    mode_transition_ = 0.0;
    update();
}

void WeatherMapWidget::setForecastTimeMin(double minutes)
{
    forecast_time_min_ = std::max(0.0, minutes);
    update();
}

void WeatherMapWidget::setViewportChangedCallback(std::function<void(double, double, double, double)> callback)
{
    viewport_changed_ = std::move(callback);
}

void WeatherMapWidget::setSelectedHazardId(std::string id)
{
    selected_hazard_id_ = std::move(id);
    update();
}

void WeatherMapWidget::resetView()
{
    target_zoom_ = 1.0;
    target_pan_ = QPointF(0.0, 0.0);
    update();
    notifyViewportChanged();
}

// ---------------------------------------------------------------------------
// Animation clock
// ---------------------------------------------------------------------------

void WeatherMapWidget::onAnimationTick()
{
    const qint64 now = clock_.elapsed();
    double dt = static_cast<double>(now - last_tick_ms_) / 1000.0;
    last_tick_ms_ = now;
    if (dt <= 0.0) {
        dt = 1.0 / 60.0;
    }
    dt = std::min(dt, 0.1);

    const bool anim = isAnimating();
    if (!anim && !pending_settle_) {
        return;
    }
    stepAnimation(dt);
    pending_settle_ = anim;
    update();
}

bool WeatherMapWidget::isAnimating() const
{
    if (dragging_) {
        return true;
    }
    if (std::abs(target_zoom_ - zoom_) > 1e-4) {
        return true;
    }
    if (QLineF(pan_, target_pan_).length() > 0.15) {
        return true;
    }
    if (mode_transition_ < 1.0 || weather_fade_ < 1.0 || map_fade_ < 1.0) {
        return true;
    }
    for (const auto& entry : layer_alpha_) {
        const double target = isHazardVisible(entry.first) ? 1.0 : 0.0;
        if (std::abs(target - entry.second) > 1e-3) {
            return true;
        }
    }
    // The precipitation drift is the only continuously-moving weather layer.
    if (weather_mode_ == WeatherMode::Precipitation && !live_hazards_.empty()) {
        return true;
    }
    return false;
}

void WeatherMapWidget::stepAnimation(double dt)
{
    animation_phase_ = std::fmod(animation_phase_ + dt, 7200.0);

    const double k_view = 1.0 - std::exp(-16.0 * dt);
    zoom_ += (target_zoom_ - zoom_) * k_view;
    pan_ += (target_pan_ - pan_) * k_view;
    if (std::abs(target_zoom_ - zoom_) < 1e-4) {
        zoom_ = target_zoom_;
    }
    if (QLineF(pan_, target_pan_).length() < 0.15) {
        pan_ = target_pan_;
    }

    if (mode_transition_ < 1.0) {
        mode_transition_ = std::min(1.0, mode_transition_ + dt / 0.30);
    }
    if (weather_fade_ < 1.0) {
        weather_fade_ = std::min(1.0, weather_fade_ + dt / 0.45);
    }
    if (map_fade_ < 1.0) {
        map_fade_ = std::min(1.0, map_fade_ + dt / 0.55);
    }

    const double k_layer = 1.0 - std::exp(-13.0 * dt);
    for (auto& entry : layer_alpha_) {
        const double target = isHazardVisible(entry.first) ? 1.0 : 0.0;
        entry.second += (target - entry.second) * k_layer;
        if (std::abs(target - entry.second) < 1e-3) {
            entry.second = target;
        }
    }
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void WeatherMapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBackground);

    ensureBaseCache();
    const GeoBounds geo = bounds();
    const QRectF map_rect = mapRect();

    QPainterPath clip;
    clip.addRoundedRect(map_rect, 12, 12);

    painter.save();
    painter.setClipPath(clip);

    if (!base_cache_.isNull()) {
        const QPointF centre = map_rect.center();
        const QPointF off_cache(kCacheMargin + centre.x() + cache_pan_.x(),
                                kCacheMargin + centre.y() + cache_pan_.y());
        const QPointF off_now(centre.x() + pan_.x(), centre.y() + pan_.y());
        const double scale = (cache_zoom_ > 1e-6) ? zoom_ / cache_zoom_ : 1.0;

        QTransform transform;
        transform.translate(off_now.x(), off_now.y());
        transform.scale(scale, scale);
        transform.translate(-off_cache.x(), -off_cache.y());

        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setOpacity(map_fade_);
        painter.setTransform(transform);
        painter.drawPixmap(0, 0, base_cache_);
        painter.restore();
    }

    const bool has_base = base_map_ && !base_map_->empty();
    const bool has_weather = !live_hazards_.empty();

    if (!has_base && !has_weather && !scenario_) {
        painter.restore();
        painter.setPen(QPen(kFrame, 1.4));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(map_rect, 12, 12);
        drawEmptyState(painter);
        return;
    }

    if (mode_transition_ < 1.0) {
        drawWeatherMode(painter, geo, prev_weather_mode_, weather_fade_ * (1.0 - mode_transition_));
        drawWeatherMode(painter, geo, weather_mode_, weather_fade_ * mode_transition_);
    } else {
        drawWeatherMode(painter, geo, weather_mode_, weather_fade_);
    }

    painter.restore();

    painter.setPen(QPen(kFrame, 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(map_rect, 12, 12);

    drawGridLabels(painter, geo);
    drawScaleBar(painter, geo);
    drawCursorReadout(painter);
}

void WeatherMapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    invalidateBaseCache();
    notifyViewportChanged();
}

void WeatherMapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    dragging_ = true;
    moved_since_press_ = false;
    last_mouse_ = event->position().toPoint();
}

void WeatherMapWidget::mouseMoveEvent(QMouseEvent* event)
{
    hover_pos_ = event->position();
    hover_valid_ = true;

    if (dragging_) {
        const QPoint current = event->position().toPoint();
        const QPoint delta = current - last_mouse_;
        if (std::abs(delta.x()) > 1 || std::abs(delta.y()) > 1) {
            moved_since_press_ = true;
        }
        pan_ += QPointF(delta);
        target_pan_ = pan_;
        last_mouse_ = current;
        update();
        return;
    }

    const WeatherHazard* hazard = hazardAt(event->position().toPoint());
    if (hazard) {
        QToolTip::showText(event->globalPosition().toPoint(), hazardTooltip(*hazard), this);
    } else {
        QToolTip::hideText();
    }
    update();
}

void WeatherMapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    dragging_ = false;
    if (!moved_since_press_) {
        const WeatherHazard* hazard = hazardAt(event->position().toPoint());
        selected_hazard_id_ = hazard ? hazard->id : std::string{};
    }
    notifyViewportChanged();
    update();
}

void WeatherMapWidget::wheelEvent(QWheelEvent* event)
{
    if (event->angleDelta().y() == 0) {
        return;
    }
    const double factor = event->angleDelta().y() > 0 ? 1.18 : 1.0 / 1.18;
    const double old_zoom = target_zoom_;
    const double new_zoom = std::clamp(old_zoom * factor, 0.55, 7.0);
    if (std::abs(new_zoom - old_zoom) < 1e-6) {
        return;
    }

    // Zoom around the cursor: keep the geographic point under the pointer fixed.
    const QPointF cursor = event->position();
    const QPointF centre = mapRect().center();
    const QPointF view_vector = (cursor - centre - target_pan_) / old_zoom;
    target_pan_ = cursor - centre - view_vector * new_zoom;
    target_zoom_ = new_zoom;

    notifyViewportChanged();
    update();
}

void WeatherMapWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    hover_valid_ = false;
    update();
}

// ---------------------------------------------------------------------------
// Projection
// ---------------------------------------------------------------------------

QRectF WeatherMapWidget::mapRect() const
{
    return QRectF(rect()).adjusted(kMargin, kMargin, -kMargin, -kMargin);
}

WeatherMapWidget::GeoBounds WeatherMapWidget::bounds() const
{
    return data_bounds_;
}

void WeatherMapWidget::recomputeDataBounds()
{
    double min_lat = std::numeric_limits<double>::max();
    double max_lat = std::numeric_limits<double>::lowest();
    double min_lon = std::numeric_limits<double>::max();
    double max_lon = std::numeric_limits<double>::lowest();
    bool any = false;

    auto include = [&](GeoPoint point) {
        min_lat = std::min(min_lat, point.lat_deg);
        max_lat = std::max(max_lat, point.lat_deg);
        min_lon = std::min(min_lon, point.lon_deg);
        max_lon = std::max(max_lon, point.lon_deg);
        any = true;
    };

    // The projection is anchored to the SXF base map whenever one is loaded so
    // that incoming live weather never shifts, rescales or "flips" the map.
    if (base_map_ && !base_map_->empty()) {
        for (const GisLayer& layer : base_map_->layers) {
            for (const GisFeature& feature : layer.features) {
                for (const auto& part : feature.parts) {
                    for (GeoPoint point : part) {
                        include(point);
                    }
                }
            }
        }
    } else {
        for (const WeatherHazard& hazard : live_hazards_) {
            for (GeoPoint point : hazard.polygon) {
                include(point);
            }
        }
        for (const LiveWeatherSample& sample : live_samples_) {
            include(sample.position);
        }
    }

    GeoBounds next{55.2, 57.2, 36.8, 38.8};
    if (any) {
        const double lat_span = std::max(0.2, max_lat - min_lat);
        const double lon_span = std::max(0.2, max_lon - min_lon);
        next = GeoBounds{
            min_lat - lat_span * 0.05,
            max_lat + lat_span * 0.05,
            min_lon - lon_span * 0.05,
            max_lon + lon_span * 0.05,
        };
    }

    const bool changed = next.min_lat != data_bounds_.min_lat
        || next.max_lat != data_bounds_.max_lat
        || next.min_lon != data_bounds_.min_lon
        || next.max_lon != data_bounds_.max_lon;
    data_bounds_ = next;
    if (changed) {
        invalidateBaseCache();
    }
}

// Aspect-correct equirectangular projection. One degree of longitude is
// cos(latitude) shorter than one degree of latitude, so the longitude axis is
// compressed, and the geographic box is fit into the map rect with a single
// uniform scale (letterboxed). Without this the SXF sheets look stretched and
// skewed, especially when zoomed out.
QPointF WeatherMapWidget::toScreen(GeoPoint point, const GeoBounds& geo_bounds) const
{
    const QRectF map_rect = mapRect();
    const double lon_span = std::max(0.0001, geo_bounds.max_lon - geo_bounds.min_lon);
    const double lat_span = std::max(0.0001, geo_bounds.max_lat - geo_bounds.min_lat);
    const double mean_lat = (geo_bounds.min_lat + geo_bounds.max_lat) * 0.5;
    const double cos_lat = std::clamp(std::cos(deg_to_rad(mean_lat)), 0.05, 1.0);

    const double geo_w = lon_span * cos_lat;
    const double geo_h = lat_span;
    const double fit = std::min(map_rect.width() / geo_w, map_rect.height() / geo_h);

    const double gx = (point.lon_deg - geo_bounds.min_lon) * cos_lat - geo_w * 0.5;
    const double gy = (geo_bounds.max_lat - point.lat_deg) - geo_h * 0.5;
    const QPointF centered(gx * fit, gy * fit);
    return map_rect.center() + centered * zoom_ + pan_;
}

GeoPoint WeatherMapWidget::fromScreen(QPointF point, const GeoBounds& geo_bounds) const
{
    const QRectF map_rect = mapRect();
    const double lon_span = std::max(0.0001, geo_bounds.max_lon - geo_bounds.min_lon);
    const double lat_span = std::max(0.0001, geo_bounds.max_lat - geo_bounds.min_lat);
    const double mean_lat = (geo_bounds.min_lat + geo_bounds.max_lat) * 0.5;
    const double cos_lat = std::clamp(std::cos(deg_to_rad(mean_lat)), 0.05, 1.0);

    const double geo_w = lon_span * cos_lat;
    const double geo_h = lat_span;
    const double fit = std::max(1e-6, std::min(map_rect.width() / geo_w, map_rect.height() / geo_h));

    const QPointF centered = (point - map_rect.center() - pan_) / std::max(0.0001, zoom_);
    const double gx = centered.x() / fit + geo_w * 0.5;
    const double gy = centered.y() / fit + geo_h * 0.5;
    return GeoPoint{
        geo_bounds.max_lat - gy,
        geo_bounds.min_lon + gx / cos_lat,
    };
}

WeatherMapWidget::GeoBounds WeatherMapWidget::visibleGeoBounds() const
{
    const GeoBounds data = bounds();
    const QRectF map_rect = mapRect();
    const GeoPoint a = fromScreen(map_rect.topLeft(), data);
    const GeoPoint b = fromScreen(map_rect.bottomRight(), data);
    return GeoBounds{
        std::min(a.lat_deg, b.lat_deg),
        std::max(a.lat_deg, b.lat_deg),
        std::min(a.lon_deg, b.lon_deg),
        std::max(a.lon_deg, b.lon_deg),
    };
}

QPolygonF WeatherMapWidget::polygonToScreen(const std::vector<GeoPoint>& polygon, const GeoBounds& geo_bounds) const
{
    QPolygonF out;
    out.reserve(static_cast<int>(polygon.size()));
    for (GeoPoint point : polygon) {
        out << toScreen(point, geo_bounds);
    }
    return out;
}

void WeatherMapWidget::notifyViewportChanged()
{
    if (!viewport_changed_) {
        return;
    }
    const GeoBounds visible = visibleGeoBounds();
    viewport_changed_(visible.min_lat, visible.max_lat, visible.min_lon, visible.max_lon);
}

// ---------------------------------------------------------------------------
// Base map cache
// ---------------------------------------------------------------------------

void WeatherMapWidget::invalidateBaseCache()
{
    base_cache_dirty_ = true;
}

void WeatherMapWidget::ensureBaseCache()
{
    const QSize widget_size = size();
    if (widget_size.isEmpty()) {
        return;
    }

    const bool view_settled = !dragging_
        && std::abs(zoom_ - target_zoom_) < 1e-4
        && QLineF(pan_, target_pan_).length() < 0.2;

    bool rebuild = base_cache_.isNull()
        || base_cache_dirty_
        || cache_size_ != widget_size
        || cache_map_ != base_map_.get();

    if (!rebuild && view_settled) {
        if (std::abs(zoom_ - cache_zoom_) > 1e-3) {
            rebuild = true;
        }
        if (QLineF(pan_, cache_pan_).length() > 0.5) {
            rebuild = true;
        }
    }
    // Rebuild mid-drag if the view has slid past the cached margin.
    if (!rebuild && QLineF(pan_, cache_pan_).length() > (kCacheMargin - 8)) {
        rebuild = true;
    }
    if (!rebuild) {
        return;
    }

    const qreal dpr = devicePixelRatioF();
    const QSize logical = widget_size + QSize(2 * kCacheMargin, 2 * kCacheMargin);
    const QSize device(static_cast<int>(std::lround(logical.width() * dpr)),
                       static_cast<int>(std::lround(logical.height() * dpr)));
    if (base_cache_.size() != device) {
        base_cache_ = QPixmap(device);
    }
    base_cache_.setDevicePixelRatio(dpr);
    base_cache_.fill(Qt::transparent);

    QPainter painter(&base_cache_);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(kCacheMargin, kCacheMargin);

    const QRectF canvas(-kCacheMargin, -kCacheMargin,
                        widget_size.width() + 2.0 * kCacheMargin,
                        widget_size.height() + 2.0 * kCacheMargin);
    const GeoBounds geo = bounds();
    renderBaseInto(painter, geo, canvas);
    renderSxfInto(painter, geo, canvas);
    painter.end();

    cache_zoom_ = zoom_;
    cache_pan_ = pan_;
    cache_size_ = widget_size;
    cache_map_ = base_map_.get();
    base_cache_dirty_ = false;
}

void WeatherMapWidget::renderBaseInto(QPainter& painter, const GeoBounds& geo, const QRectF& canvas)
{
    QLinearGradient terrain(canvas.topLeft(), canvas.bottomRight());
    terrain.setColorAt(0.0, QColor("#11212d"));
    terrain.setColorAt(0.55, QColor("#0d1a26"));
    terrain.setColorAt(1.0, QColor("#141a24"));
    painter.fillRect(canvas, terrain);

    const double lat_step = niceGridStep(geo.max_lat - geo.min_lat);
    const double lon_step = niceGridStep(geo.max_lon - geo.min_lon);

    painter.setPen(QPen(QColor(86, 116, 142, 58), 1.0));
    const double first_lat = std::ceil(geo.min_lat / lat_step) * lat_step;
    for (double lat = first_lat; lat <= geo.max_lat; lat += lat_step) {
        painter.drawLine(toScreen({lat, geo.min_lon - 6.0}, geo),
                         toScreen({lat, geo.max_lon + 6.0}, geo));
    }
    const double first_lon = std::ceil(geo.min_lon / lon_step) * lon_step;
    for (double lon = first_lon; lon <= geo.max_lon; lon += lon_step) {
        painter.drawLine(toScreen({geo.min_lat - 6.0, lon}, geo),
                         toScreen({geo.max_lat + 6.0, lon}, geo));
    }
}

void WeatherMapWidget::renderSxfInto(QPainter& painter, const GeoBounds& geo, const QRectF& canvas)
{
    if (!base_map_ || base_map_->empty()) {
        return;
    }

    constexpr double kLodPx = 1.7;

    // Pass 0: polygon fills, pass 1: line work — so roads sit above area fills.
    for (int pass = 0; pass < 2; ++pass) {
        const GisGeometryType want = (pass == 0) ? GisGeometryType::Polygon : GisGeometryType::Line;
        for (const GisLayer& layer : base_map_->layers) {
            const SxfLayerStyle style = styleForSxfLayer(layer.name);
            bool pen_ready = false;

            for (const GisFeature& feature : layer.features) {
                if (feature.type != want) {
                    continue;
                }
                for (const auto& part : feature.parts) {
                    if (want == GisGeometryType::Polygon && part.size() < 3) {
                        continue;
                    }
                    if (want == GisGeometryType::Line && part.size() < 2) {
                        continue;
                    }

                    const QPolygonF poly = polygonToScreen(part, geo);
                    const QRectF box = poly.boundingRect();
                    if (!box.intersects(canvas)) {
                        continue; // viewport culling
                    }
                    if (box.width() < kLodPx && box.height() < kLodPx) {
                        continue; // level-of-detail: drop sub-pixel clutter
                    }

                    if (!pen_ready) {
                        if (want == GisGeometryType::Polygon) {
                            painter.setPen(QPen(style.line, style.width, style.pen_style));
                            painter.setBrush(style.fill_polygons ? QBrush(style.fill) : Qt::NoBrush);
                        } else {
                            painter.setPen(QPen(style.line, style.width, style.pen_style,
                                                Qt::RoundCap, Qt::RoundJoin));
                            painter.setBrush(Qt::NoBrush);
                        }
                        pen_ready = true;
                    }

                    if (want == GisGeometryType::Polygon) {
                        painter.drawPolygon(poly);
                    } else {
                        painter.drawPolyline(poly);
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Per-frame drawing
// ---------------------------------------------------------------------------

void WeatherMapWidget::drawGridLabels(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    const double lat_step = niceGridStep(geo.max_lat - geo.min_lat);
    const double lon_step = niceGridStep(geo.max_lon - geo.min_lon);

    QFont font = painter.font();
    font.setPointSizeF(8.5);
    painter.setFont(font);
    painter.setPen(kGridLabel);

    const double first_lat = std::ceil(geo.min_lat / lat_step) * lat_step;
    for (double lat = first_lat; lat <= geo.max_lat; lat += lat_step) {
        const double y = toScreen({lat, geo.min_lon}, geo).y();
        if (y < map_rect.top() + 40 || y > map_rect.bottom() - 8) {
            continue;
        }
        painter.drawText(QPointF(map_rect.left() + 8, y - 3),
                         QString::number(lat, 'f', 1) + QChar(0x00B0));
    }

    const double first_lon = std::ceil(geo.min_lon / lon_step) * lon_step;
    for (double lon = first_lon; lon <= geo.max_lon; lon += lon_step) {
        const double x = toScreen({geo.min_lat, lon}, geo).x();
        if (x < map_rect.left() + 8 || x > map_rect.right() - 34) {
            continue;
        }
        painter.drawText(QPointF(x + 4, map_rect.bottom() - 8),
                         QString::number(lon, 'f', 1) + QChar(0x00B0));
    }
}

void WeatherMapWidget::drawWeatherMode(QPainter& painter, const GeoBounds& geo, WeatherMode mode, double alpha)
{
    if (alpha <= 0.01) {
        return;
    }
    painter.save();
    painter.setOpacity(painter.opacity() * alpha);
    switch (mode) {
    case WeatherMode::Precipitation:
        drawPrecipitation(painter, geo);
        break;
    case WeatherMode::Clouds:
        drawClouds(painter, geo);
        break;
    case WeatherMode::Temperature:
        drawTemperature(painter, geo);
        break;
    case WeatherMode::Wind:
        drawWind(painter, geo);
        break;
    case WeatherMode::Visibility:
        drawVisibility(painter, geo);
        break;
    case WeatherMode::Pressure:
        drawPressure(painter, geo);
        break;
    }
    painter.restore();
}

void WeatherMapWidget::drawPrecipitation(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    const std::vector<WeatherHazard>* hazards = activeHazards();
    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0 || forecast_time_min_ > sample.valid_to_min + 25.0
            || sample.precipitation_mm <= 0.01) {
            continue;
        }
        const QPointF p = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-60, -40, 60, 40).contains(p)) {
            continue;
        }
        QColor fill("#5d8ee8");
        fill.setAlpha(static_cast<int>(std::clamp(44.0 + sample.precipitation_mm * 22.0, 52.0, 128.0)));
        painter.setPen(QPen(QColor(157, 194, 255, 165), 1.2));
        painter.setBrush(fill);
        painter.drawEllipse(p, 44.0, 28.0);
        if (sample.cape_jkg >= 800.0) {
            painter.setPen(QPen(QColor(240, 246, 255, 180), 2.0));
            painter.drawLine(QPointF(p.x(), p.y() - 10), QPointF(p.x() - 7, p.y() + 2));
            painter.drawLine(QPointF(p.x() - 7, p.y() + 2), QPointF(p.x() + 2, p.y() + 2));
            painter.drawLine(QPointF(p.x() + 2, p.y() + 2), QPointF(p.x() - 4, p.y() + 12));
        }
    }

    if (hazards) {
        const double outer = painter.opacity();
        for (const WeatherHazard& hazard : *hazards) {
            if (hazard.type != HazardType::Thunderstorm && hazard.type != HazardType::HeavyPrecipitation) {
                continue;
            }
            if (!hazardVisibleAtTime(hazard) || hazard.polygon.size() < 3) {
                continue;
            }
            const double la = layerAlpha(hazard.type);
            if (la <= 0.02) {
                continue;
            }
            painter.setOpacity(outer * la);

            QPolygonF polygon = polygonToScreen(hazard.polygon, geo);
            polygon.translate(QPointF(forecast_time_min_ * 0.25 + animation_phase_ * 8.0,
                                      -forecast_time_min_ * 0.08));
            const QPainterPath polygon_path = pathFromPolygon(polygon);

            QColor fill = hazard.type == HazardType::Thunderstorm ? QColor("#5d8ee8") : QColor("#3f83d7");
            fill.setAlpha(120);
            painter.setPen(QPen(QColor(157, 194, 255, 185), 1.4));
            painter.setBrush(fill);
            painter.drawPolygon(polygon);

            const QRectF area = polygon.boundingRect();
            painter.setPen(QPen(QColor(227, 238, 255, 170), 2.0));
            for (double y = area.top() + 18; y < area.bottom(); y += 24) {
                for (double x = area.left() + 16; x < area.right(); x += 26) {
                    const QPointF p(x + std::fmod(forecast_time_min_ * 0.7 + animation_phase_ * 18.0, 18.0), y);
                    if (!polygon_path.contains(p)) {
                        continue;
                    }
                    painter.drawLine(QPointF(p.x(), p.y() - 7), QPointF(p.x() - 5, p.y() + 1));
                    painter.drawLine(QPointF(p.x() - 5, p.y() + 1), QPointF(p.x() + 2, p.y() + 1));
                    painter.drawLine(QPointF(p.x() + 2, p.y() + 1), QPointF(p.x() - 3, p.y() + 9));
                }
            }
        }
        painter.setOpacity(outer);
    }
    painter.restore();
}

void WeatherMapWidget::drawClouds(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    QFont font = painter.font();
    font.setPointSizeF(8.8);
    font.setBold(true);
    painter.setFont(font);

    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0
            || forecast_time_min_ > sample.valid_to_min + 25.0) {
            continue;
        }
        const QPointF p = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-66, -44, 66, 44).contains(p)) {
            continue;
        }
        const double cover = std::clamp(sample.cloud_cover_pct, 0.0, 100.0);
        const double radius = 22.0 + cover * 0.28;
        QColor fill(208, 216, 228);
        fill.setAlpha(static_cast<int>(26.0 + cover * 1.45));
        painter.setPen(QPen(QColor(150, 165, 184, 150), 1.1));
        painter.setBrush(fill);
        painter.drawEllipse(p, radius, radius * 0.64);
        painter.setPen(cover >= 45.0 ? QColor("#16202b") : QColor("#c6d2df"));
        painter.drawText(QRectF(p.x() - 36, p.y() - 10, 72, 20), Qt::AlignCenter,
                         QString("%1%").arg(cover, 0, 'f', 0));
    }
    painter.restore();
}

void WeatherMapWidget::drawVisibility(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);
    QFont font = painter.font();
    font.setPointSizeF(8.8);
    font.setBold(true);
    painter.setFont(font);
    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0 || forecast_time_min_ > sample.valid_to_min + 25.0
            || sample.visibility_m <= 0.0) {
            continue;
        }
        const QPointF p = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-52, -24, 52, 24).contains(p)) {
            continue;
        }
        QColor fill("#d4cdb4");
        fill.setAlpha(sample.visibility_m < 5000.0 ? 94 : 42);
        painter.setPen(QPen(QColor(230, 224, 198, 150), 1.2));
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(p.x() - 34, p.y() - 12, 68, 24), 7, 7);
        painter.setPen(QColor("#fff8df"));
        painter.drawText(QRectF(p.x() - 34, p.y() - 12, 68, 24), Qt::AlignCenter,
                         QString("%1 км").arg(sample.visibility_m / 1000.0, 0, 'f', 1));
    }
    painter.restore();
    drawModeHazards(painter, geo, HazardType::LowVisibility, QColor("#d5d0bd"), "вид.");
}

void WeatherMapWidget::drawWind(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    QFont font = painter.font();
    font.setPointSizeF(8.6);
    font.setBold(true);
    painter.setFont(font);

    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0
            || forecast_time_min_ > sample.valid_to_min + 25.0) {
            continue;
        }
        const QPointF start = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-34, -34, 34, 34).contains(start)) {
            continue;
        }
        const double speed = std::max(0.0, sample.wind_speed_ms);
        const double gust = std::max(speed, sample.wind_gust_ms);

        // Open-Meteo wind_direction_10m is the meteorological bearing the wind
        // blows FROM; the arrow shows the direction the air actually moves.
        const double to_bearing = deg_to_rad(sample.wind_direction_deg + 180.0);
        const QPointF dir(std::sin(to_bearing), -std::cos(to_bearing));
        const double len = std::clamp(16.0 + speed * 2.6, 16.0, 62.0);
        const QPointF end = start + dir * len;

        QColor stroke = gust >= 17.0 ? QColor("#ffd05c") : QColor("#cfe2ff");
        stroke.setAlpha(215);
        painter.setPen(QPen(stroke, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(start, end);

        const QPointF back = end - dir * 8.0;
        const QPointF side(-dir.y() * 5.0, dir.x() * 5.0);
        painter.drawLine(end, back + side);
        painter.drawLine(end, back - side);

        painter.setPen(Qt::NoPen);
        painter.setBrush(stroke);
        painter.drawEllipse(start, 2.6, 2.6);

        const QString label = QString("%1 м/с").arg(speed, 0, 'f', 0);
        const QRectF chip(start.x() + 7.0, start.y() + 6.0, 50.0, 18.0);
        painter.setBrush(QColor(12, 20, 29, 175));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(chip, 6, 6);
        painter.setPen(QColor("#eaf4ff"));
        painter.drawText(chip, Qt::AlignCenter, label);
    }
    painter.restore();
}

void WeatherMapWidget::drawPressure(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    QFont font = painter.font();
    font.setPointSizeF(9.2);
    font.setBold(true);
    painter.setFont(font);

    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0 || forecast_time_min_ > sample.valid_to_min + 25.0
            || sample.pressure_hpa <= 0.0) {
            continue;
        }
        const QPointF p = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-50, -20, 50, 20).contains(p)) {
            continue;
        }
        const QString label = QString::number(sample.pressure_hpa, 'f', 0);
        const QRectF chip(p.x() - 26, p.y() - 12, 52, 24);
        painter.setPen(QPen(QColor(239, 224, 166, 150), 1.2));
        painter.setBrush(QColor(42, 38, 25, 150));
        painter.drawRoundedRect(chip, 7, 7);
        painter.setPen(QColor("#f2e5ae"));
        painter.drawText(chip, Qt::AlignCenter, label);
    }
    painter.restore();
}

void WeatherMapWidget::drawTemperature(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    QFont font = painter.font();
    font.setPointSizeF(9.0);
    font.setBold(true);
    painter.setFont(font);

    auto mix = [](const QColor& a, const QColor& b, double t) {
        t = std::clamp(t, 0.0, 1.0);
        return QColor(
            static_cast<int>(a.red() + (b.red() - a.red()) * t),
            static_cast<int>(a.green() + (b.green() - a.green()) * t),
            static_cast<int>(a.blue() + (b.blue() - a.blue()) * t));
    };

    for (const LiveWeatherSample& sample : live_samples_) {
        if (forecast_time_min_ < sample.valid_from_min - 25.0
            || forecast_time_min_ > sample.valid_to_min + 25.0) {
            continue;
        }
        const QPointF p = toScreen(sample.position, geo);
        if (!map_rect.adjusted(-48, -26, 48, 26).contains(p)) {
            continue;
        }

        const double t = sample.temperature_c;
        // -30 °C (blue) -> +5 °C (green) -> +40 °C (red)
        const double k = std::clamp((t + 30.0) / 70.0, 0.0, 1.0);
        const QColor cold(86, 140, 214);
        const QColor mild(96, 174, 112);
        const QColor warm(222, 96, 60);
        const QColor fill = k < 0.5 ? mix(cold, mild, k * 2.0) : mix(mild, warm, (k - 0.5) * 2.0);
        QColor disk = fill;
        disk.setAlpha(165);
        painter.setPen(QPen(fill, 1.4));
        painter.setBrush(disk);
        painter.drawRoundedRect(QRectF(p.x() - 28.0, p.y() - 13.0, 56.0, 26.0), 8, 8);
        painter.setPen(QColor("#0e151c"));
        painter.drawText(QRectF(p.x() - 28.0, p.y() - 13.0, 56.0, 26.0), Qt::AlignCenter,
                         QString("%1°").arg(t, 0, 'f', 0));
    }
    painter.restore();
}

void WeatherMapWidget::drawModeHazards(QPainter& painter, const GeoBounds& geo, HazardType type, const QColor& color, const QString& label)
{
    const QRectF map_rect = mapRect();
    painter.save();
    painter.setClipRect(map_rect, Qt::IntersectClip);

    const std::vector<WeatherHazard>* hazards = activeHazards();
    if (hazards) {
        const double outer = painter.opacity();
        for (const WeatherHazard& hazard : *hazards) {
            if (hazard.type != type || !hazardVisibleAtTime(hazard) || hazard.polygon.size() < 3) {
                continue;
            }
            const double la = layerAlpha(hazard.type);
            if (la <= 0.02) {
                continue;
            }
            painter.setOpacity(outer * la);

            const QPolygonF polygon = polygonToScreen(hazard.polygon, geo);
            QColor fill = color;
            fill.setAlpha(96);
            QColor stroke = color.darker(130);
            stroke.setAlpha(185);
            painter.setPen(QPen(stroke, 1.5));
            painter.setBrush(fill);
            painter.drawPolygon(polygon);

            const QRectF box = polygon.boundingRect();
            if (box.width() > 64 && box.height() > 24) {
                painter.setPen(QColor("#f4f8ff"));
                painter.drawText(box, Qt::AlignCenter, label);
            }
        }
        painter.setOpacity(outer);
    }
    painter.restore();
}

void WeatherMapWidget::drawRoute(QPainter& painter, const GeoBounds& geo)
{
    if (!scenario_ || scenario_->route.waypoints.size() < 2) {
        return;
    }

    QPainterPath route_path;
    const auto& waypoints = scenario_->route.waypoints;
    route_path.moveTo(toScreen(waypoints.front().position, geo));
    for (std::size_t i = 1; i < waypoints.size(); ++i) {
        route_path.lineTo(toScreen(waypoints[i].position, geo));
    }

    painter.save();
    painter.setClipRect(mapRect(), Qt::IntersectClip);

    painter.setPen(QPen(QColor(64, 150, 240, 52),
                        std::max(12.0, scenario_->route_corridor_nm * 0.7),
                        Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(route_path);

    painter.setPen(QPen(QColor("#3f8fe0"), 3.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(route_path);

    painter.setBrush(QColor("#f4f8ff"));
    painter.setPen(QPen(QColor("#3f8fe0"), 2));
    for (const Waypoint& waypoint : waypoints) {
        const QPointF point = toScreen(waypoint.position, geo);
        painter.drawEllipse(point, 5.0, 5.0);
        painter.setPen(QColor("#cdd9e6"));
        painter.drawText(QRectF(point.x() + 9, point.y() - 16, 110, 22),
                         Qt::AlignLeft | Qt::AlignVCenter, qstr(waypoint.name));
        painter.setPen(QPen(QColor("#3f8fe0"), 2));
    }

    painter.restore();
}

void WeatherMapWidget::drawAirports(QPainter& painter, const GeoBounds& geo)
{
    if (!scenario_) {
        return;
    }

    painter.save();
    painter.setClipRect(mapRect(), Qt::IntersectClip);
    painter.setBrush(QColor("#18283f"));
    painter.setPen(QPen(QColor("#f4f8ff"), 2));

    for (const AirportWeather& airport : scenario_->airports) {
        auto it = std::find_if(
            scenario_->route.waypoints.begin(),
            scenario_->route.waypoints.end(),
            [&airport](const Waypoint& waypoint) { return waypoint.name == airport.icao; });
        if (it == scenario_->route.waypoints.end()) {
            continue;
        }
        const QPointF point = toScreen(it->position, geo);
        painter.drawRect(QRectF(point.x() - 6, point.y() - 6, 12, 12));
    }

    painter.restore();
}

void WeatherMapWidget::drawRiskBadge(QPainter& painter)
{
    if (!report_) {
        return;
    }

    const QString text = QString("Риск: %1  %2/100")
        .arg(riskLevelRu(report_->level))
        .arg(report_->score, 0, 'f', 1);

    QFont font = painter.font();
    font.setBold(true);
    font.setPointSizeF(11.5);
    painter.setFont(font);

    const QFontMetricsF fm(font);
    const double width = fm.horizontalAdvance(text) + 30;
    const QRectF badge(mapRect().right() - width - 14, mapRect().top() + 14, width, 32);
    painter.setPen(Qt::NoPen);
    painter.setBrush(riskColor(report_->level));
    painter.drawRoundedRect(badge, 9, 9);
    painter.setPen(Qt::white);
    painter.drawText(badge, Qt::AlignCenter, text);
}

void WeatherMapWidget::drawScaleBar(QPainter& painter, const GeoBounds& geo)
{
    const QRectF map_rect = mapRect();
    const GeoPoint a = fromScreen(QPointF(map_rect.center().x() - 50.0, map_rect.center().y()), geo);
    const GeoPoint b = fromScreen(QPointF(map_rect.center().x() + 50.0, map_rect.center().y()), geo);
    const double nm_per_100px = haversine_nm(a, b);
    if (!std::isfinite(nm_per_100px) || nm_per_100px <= 0.0) {
        return;
    }

    const double nm_per_px = nm_per_100px / 100.0;
    const double target_nm = nm_per_px * 130.0;
    static const double kSteps[] = {1, 2, 5, 10, 20, 25, 50, 100, 150, 200, 300, 500, 750};
    double nice = kSteps[0];
    for (double step : kSteps) {
        if (step <= target_nm) {
            nice = step;
        }
    }
    const double bar_px = nice / nm_per_px;
    if (!std::isfinite(bar_px) || bar_px < 12.0 || bar_px > map_rect.width()) {
        return;
    }

    const double x = map_rect.left() + 16.0;
    const double y = map_rect.bottom() - 22.0;
    painter.setPen(QPen(QColor(232, 238, 246, 210), 2.2));
    painter.drawLine(QPointF(x, y), QPointF(x + bar_px, y));
    painter.drawLine(QPointF(x, y - 4), QPointF(x, y + 4));
    painter.drawLine(QPointF(x + bar_px, y - 4), QPointF(x + bar_px, y + 4));

    QFont font = painter.font();
    font.setPointSizeF(8.5);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(QColor(216, 224, 234));
    painter.drawText(QRectF(x, y - 22, bar_px + 80, 16), Qt::AlignLeft | Qt::AlignVCenter,
                     QString::number(nice, 'f', 0) + " NM");
}

void WeatherMapWidget::drawCursorReadout(QPainter& painter)
{
    if (!hover_valid_) {
        return;
    }
    const GeoPoint geo = fromScreen(hover_pos_, bounds());

    const QString text = QString("%1%3N   %2%3E   ×%4")
        .arg(geo.lat_deg, 0, 'f', 3)
        .arg(geo.lon_deg, 0, 'f', 3)
        .arg(QChar(0x00B0))
        .arg(zoom_, 0, 'f', 2);

    QFont font = painter.font();
    font.setPointSizeF(9.0);
    font.setBold(false);
    painter.setFont(font);

    const QFontMetricsF fm(font);
    const double width = fm.horizontalAdvance(text) + 22;
    const QRectF pill(mapRect().left() + 12, mapRect().top() + 12, width, 24);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(11, 17, 23, 210));
    painter.drawRoundedRect(pill, 7, 7);
    painter.setPen(QColor(212, 222, 234));
    painter.drawText(pill, Qt::AlignCenter, text);
}

void WeatherMapWidget::drawEmptyState(QPainter& painter)
{
    painter.setPen(QColor("#5f6f80"));
    QFont font = painter.font();
    font.setPointSizeF(15.0);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(rect(), Qt::AlignCenter,
                     "Откройте SXF-карту: выберите масштаб 1M / 200K или папку с листами");
}

// ---------------------------------------------------------------------------
// Hazard helpers
// ---------------------------------------------------------------------------

const WeatherHazard* WeatherMapWidget::hazardAt(const QPoint& point) const
{
    const std::vector<WeatherHazard>* hazards = activeHazards();
    if (!hazards) {
        return nullptr;
    }

    const GeoBounds geo = bounds();
    for (auto it = hazards->rbegin(); it != hazards->rend(); ++it) {
        if (!isHazardVisible(it->type) || layerAlpha(it->type) <= 0.1) {
            continue;
        }
        const QPainterPath path = pathFromPolygon(polygonToScreen(it->polygon, geo));
        if (path.contains(point)) {
            return &*it;
        }
    }
    return nullptr;
}

const std::vector<WeatherHazard>* WeatherMapWidget::activeHazards() const
{
    return &live_hazards_;
}

bool WeatherMapWidget::isHazardVisible(HazardType type) const
{
    return visible_hazards_.find(type) != visible_hazards_.end();
}

double WeatherMapWidget::layerAlpha(HazardType type) const
{
    const auto it = layer_alpha_.find(type);
    return it == layer_alpha_.end() ? 1.0 : it->second;
}

bool WeatherMapWidget::hazardVisibleAtTime(const WeatherHazard& hazard) const
{
    constexpr double fade_min = 20.0;
    return forecast_time_min_ >= hazard.valid_from_min - fade_min
        && forecast_time_min_ <= hazard.valid_to_min + fade_min;
}

QColor WeatherMapWidget::hazardColor(HazardType type) const
{
    switch (type) {
    case HazardType::Thunderstorm:      return QColor("#d43d3d");
    case HazardType::Icing:             return QColor("#1c9ed6");
    case HazardType::Turbulence:        return QColor("#d78a19");
    case HazardType::LowVisibility:     return QColor("#4a6fa5");
    case HazardType::LowCeiling:        return QColor("#5c7c4f");
    case HazardType::WindShear:         return QColor("#a64ac9");
    case HazardType::VolcanicAsh:       return QColor("#6d7278");
    case HazardType::HeavyPrecipitation:return QColor("#218c74");
    case HazardType::Unknown:           return QColor("#596170");
    }
    return QColor("#596170");
}

QColor WeatherMapWidget::riskColor(RiskLevel level) const
{
    switch (level) {
    case RiskLevel::Low:     return QColor("#2f8f5b");
    case RiskLevel::Medium:  return QColor("#b98213");
    case RiskLevel::High:    return QColor("#bf4d26");
    case RiskLevel::Extreme: return QColor("#b52d44");
    }
    return QColor("#596170");
}

} // namespace aviwx::gui
