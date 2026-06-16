#pragma once

#include "aviwx/GisMap.h"
#include "aviwx/LiveWeather.h"
#include "aviwx/RiskEngine.h"
#include "aviwx/Scenario.h"

#include <QColor>
#include <QElapsedTimer>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QWidget>

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class QTimer;

namespace aviwx::gui {

enum class WeatherMode {
    Precipitation,
    Clouds,
    Temperature,
    Wind,
    Visibility,
    Pressure
};

// Aviation weather map view.
//
// Performance model:
//  - The static base layer (terrain + grid + SXF vector geometry) is rendered
//    once into an oversized QPixmap cache. While the user only animates weather
//    or pans/zooms a little, the cache is simply blitted with a transform.
//  - The cache is rebuilt only when the view settles at a new zoom, when the
//    base map changes or when the widget is resized.
//  - The animated weather overlay is the only thing redrawn every frame, so the
//    60 fps render clock stays cheap even with ~90k SXF features loaded.
class WeatherMapWidget : public QWidget {
public:
    explicit WeatherMapWidget(QWidget* parent = nullptr);

    void setScenario(const Scenario* scenario, const RiskReport* report);
    void setBaseMap(std::shared_ptr<const GisMap> map, bool reset_view = true);
    void setLiveWeather(std::vector<WeatherHazard> hazards, std::vector<LiveWeatherSample> samples = {});
    void setHazardVisible(HazardType type, bool visible);
    void setWeatherMode(WeatherMode mode);
    void setForecastTimeMin(double minutes);
    void setViewportChangedCallback(std::function<void(double, double, double, double)> callback);
    void setSelectedHazardId(std::string id);
    void resetView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct GeoBounds {
        double min_lat{};
        double max_lat{};
        double min_lon{};
        double max_lon{};
    };

    // projection / geometry
    QRectF mapRect() const;
    GeoBounds bounds() const;
    GeoBounds visibleGeoBounds() const;
    QPointF toScreen(GeoPoint point, const GeoBounds& bounds) const;
    GeoPoint fromScreen(QPointF point, const GeoBounds& bounds) const;
    QPolygonF polygonToScreen(const std::vector<GeoPoint>& polygon, const GeoBounds& bounds) const;
    void recomputeDataBounds();
    void notifyViewportChanged();

    // hazards
    const WeatherHazard* hazardAt(const QPoint& point) const;
    const std::vector<WeatherHazard>* activeHazards() const;
    bool isHazardVisible(HazardType type) const;
    double layerAlpha(HazardType type) const;
    bool hazardVisibleAtTime(const WeatherHazard& hazard) const;

    // animation clock
    void onAnimationTick();
    bool isAnimating() const;
    void stepAnimation(double dt);

    // base cache
    void invalidateBaseCache();
    void ensureBaseCache();
    void renderBaseInto(QPainter& painter, const GeoBounds& bounds, const QRectF& canvas);
    void renderSxfInto(QPainter& painter, const GeoBounds& bounds, const QRectF& canvas);

    // per-frame drawing
    void drawGridLabels(QPainter& painter, const GeoBounds& bounds);
    void drawWeatherMode(QPainter& painter, const GeoBounds& bounds, WeatherMode mode, double alpha);
    void drawPrecipitation(QPainter& painter, const GeoBounds& bounds);
    void drawClouds(QPainter& painter, const GeoBounds& bounds);
    void drawTemperature(QPainter& painter, const GeoBounds& bounds);
    void drawVisibility(QPainter& painter, const GeoBounds& bounds);
    void drawWind(QPainter& painter, const GeoBounds& bounds);
    void drawPressure(QPainter& painter, const GeoBounds& bounds);
    void drawModeHazards(QPainter& painter, const GeoBounds& bounds, HazardType type, const QColor& color, const QString& label);
    void drawRoute(QPainter& painter, const GeoBounds& bounds);
    void drawAirports(QPainter& painter, const GeoBounds& bounds);
    void drawRiskBadge(QPainter& painter);
    void drawScaleBar(QPainter& painter, const GeoBounds& bounds);
    void drawCursorReadout(QPainter& painter);
    void drawEmptyState(QPainter& painter);

    QColor hazardColor(HazardType type) const;
    QColor riskColor(RiskLevel level) const;

    // data
    const Scenario* scenario_{};
    const RiskReport* report_{};
    std::shared_ptr<const GisMap> base_map_;
    std::vector<WeatherHazard> live_hazards_;
    std::vector<LiveWeatherSample> live_samples_;
    std::set<HazardType> visible_hazards_;
    std::map<HazardType, double> layer_alpha_;
    GeoBounds data_bounds_{55.2, 57.2, 36.8, 38.8};

    // view transform
    double zoom_{1.0};
    double target_zoom_{1.0};
    QPointF pan_{0.0, 0.0};
    QPointF target_pan_{0.0, 0.0};
    bool dragging_{false};
    bool moved_since_press_{false};
    QPoint last_mouse_;
    QPointF hover_pos_{-1.0, -1.0};
    bool hover_valid_{false};

    // weather state
    WeatherMode weather_mode_{WeatherMode::Precipitation};
    WeatherMode prev_weather_mode_{WeatherMode::Precipitation};
    double mode_transition_{1.0};
    double weather_fade_{1.0};
    double map_fade_{1.0};
    double forecast_time_min_{0.0};
    double animation_phase_{0.0};

    // base pixmap cache
    QPixmap base_cache_;
    double cache_zoom_{0.0};
    QPointF cache_pan_{0.0, 0.0};
    QSize cache_size_;
    const GisMap* cache_map_{nullptr};
    bool base_cache_dirty_{true};

    // animation clock
    QTimer* anim_timer_{};
    QElapsedTimer clock_;
    qint64 last_tick_ms_{0};
    bool pending_settle_{false};

    std::function<void(double, double, double, double)> viewport_changed_;
    std::string selected_hazard_id_;
};

} // namespace aviwx::gui
