#pragma once

#include "aviwx/LiveWeather.h"
#include "aviwx/Scenario.h"

#include <QWidget>

#include <functional>
#include <vector>

class QTimer;

namespace aviwx::gui {

// Forecast time scrubber with an eased playhead, a pulsing marker and animated
// hover feedback over the hazard timeline segments.
class WeatherTimelineWidget : public QWidget {
public:
    explicit WeatherTimelineWidget(QWidget* parent = nullptr);

    void setScenario(const Scenario* scenario);
    void setLiveWeather(std::vector<WeatherHazard> hazards, std::vector<LiveWeatherSample> samples = {});
    void setForecastTimeMin(double minutes);
    void setTimeChangedCallback(std::function<void(double)> callback);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRectF trackRect() const;
    double maxTimeMin() const;
    double timeAtX(double x) const;
    double xForTime(double minutes) const;
    void setTimeFromPosition(double x);
    void onTick();

    const Scenario* scenario_{};
    std::vector<WeatherHazard> live_hazards_;
    std::vector<LiveWeatherSample> live_samples_;
    std::function<void(double)> time_changed_;
    double forecast_time_min_{0.0};
    double display_time_{0.0};
    double pulse_{0.0};
    double hover_x_{-1.0};
    bool hover_active_{false};
    bool dragging_{false};
    QTimer* timer_{};
};

} // namespace aviwx::gui
