#include "aviwx/gui/WeatherTimelineWidget.h"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <utility>

namespace aviwx::gui {
namespace {

QString timeLabel(double minutes)
{
    if (std::abs(minutes) < 0.5) {
        return "Сейчас";
    }
    return QString("+%1 мин").arg(minutes, 0, 'f', 0);
}

QColor colorForHazard(HazardType type)
{
    switch (type) {
    case HazardType::Thunderstorm:       return QColor("#ffe66f");
    case HazardType::HeavyPrecipitation: return QColor("#6aa8ff");
    case HazardType::Icing:              return QColor("#9bdcff");
    case HazardType::Turbulence:         return QColor("#e0a13a");
    default:                             return QColor("#d6e4ff");
    }
}

} // namespace

WeatherTimelineWidget::WeatherTimelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(86);
    setMaximumHeight(96);
    setMouseTracking(true);

    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, [this] { onTick(); });
    timer_->start();
}

void WeatherTimelineWidget::setScenario(const Scenario* scenario)
{
    scenario_ = scenario;
    forecast_time_min_ = 0.0;
    update();
}

void WeatherTimelineWidget::setLiveWeather(std::vector<WeatherHazard> hazards, std::vector<LiveWeatherSample> samples)
{
    live_hazards_ = std::move(hazards);
    live_samples_ = std::move(samples);
    update();
}

void WeatherTimelineWidget::setForecastTimeMin(double minutes)
{
    forecast_time_min_ = std::clamp(minutes, 0.0, maxTimeMin());
    update();
}

void WeatherTimelineWidget::setTimeChangedCallback(std::function<void(double)> callback)
{
    time_changed_ = std::move(callback);
}

void WeatherTimelineWidget::onTick()
{
    constexpr double dt = 1.0 / 60.0;
    const double k = 1.0 - std::exp(-22.0 * dt);
    const double next = display_time_ + (forecast_time_min_ - display_time_) * k;
    if (std::abs(next - display_time_) > 1e-3) {
        display_time_ = next;
    } else {
        display_time_ = forecast_time_min_;
    }
    pulse_ = std::fmod(pulse_ + dt, 3600.0);
    update();
}

void WeatherTimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#0f1822"));
    painter.setPen(QPen(QColor("#1d2935"), 1));
    painter.drawLine(QPointF(rect().left(), rect().top()),
                     QPointF(rect().right(), rect().top()));

    const QRectF track = trackRect();
    const double radius = track.height() * 0.5;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1a2533"));
    painter.drawRoundedRect(track, radius, radius);

    const double max_time = maxTimeMin();

    for (const WeatherHazard& hazard : live_hazards_) {
        const double left = xForTime(std::clamp(hazard.valid_from_min, 0.0, max_time));
        const double right = xForTime(std::clamp(hazard.valid_to_min, 0.0, max_time));
        if (right <= left) {
            continue;
        }
        const bool hovered = hover_active_ && hover_x_ >= left && hover_x_ <= right;

        QColor fill = colorForHazard(hazard.type);
        fill.setAlpha(hazard.type == HazardType::Thunderstorm ? 215 : 155);
        if (hovered) {
            fill = fill.lighter(128);
        }
        const QRectF segment(left, track.top(), std::max(15.0, right - left), track.height());
        painter.setBrush(fill);
        painter.drawRoundedRect(segment, radius, radius);
    }

    QFont label_font = painter.font();
    label_font.setPointSizeF(9.0);
    painter.setFont(label_font);
    painter.setPen(QColor("#8392a3"));
    for (int i = 0; i <= 6; ++i) {
        const double t = max_time * i / 6.0;
        const double x = xForTime(t);
        painter.setPen(QPen(QColor(110, 128, 146, 130), 1));
        painter.drawLine(QPointF(x, track.top() - 6), QPointF(x, track.top() - 1));
        painter.setPen(QColor("#8392a3"));
        painter.drawText(QRectF(x - 38, track.bottom() + 6, 76, 18), Qt::AlignCenter, timeLabel(t));
    }

    if (hover_active_ && !dragging_ && hover_x_ >= track.left() && hover_x_ <= track.right()) {
        painter.setPen(QPen(QColor(255, 255, 255, 55), 1, Qt::DashLine));
        painter.drawLine(QPointF(hover_x_, track.top() - 4), QPointF(hover_x_, track.bottom() + 4));
    }

    const double x = xForTime(display_time_);
    painter.setPen(QPen(QColor("#ffcf42"), 2.2));
    painter.drawLine(QPointF(x, 24), QPointF(x, height() - 14));

    const double glow = 13.0 + (std::sin(pulse_ * 6.0) * 0.5 + 0.5) * 6.0;
    QColor glow_color("#ffe166");
    glow_color.setAlpha(48);
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow_color);
    painter.drawEllipse(QPointF(x, track.center().y()), glow, glow);

    painter.setBrush(QColor("#ffe166"));
    painter.setPen(QPen(QColor("#10161f"), 3));
    painter.drawEllipse(QPointF(x, track.center().y()), 12, 12);
    painter.setPen(QColor("#10161f"));
    painter.drawText(QRectF(x - 16, track.center().y() - 10, 32, 20),
                     Qt::AlignCenter, QString(QChar(0x25B6)));

    QFont chip_font = painter.font();
    chip_font.setBold(true);
    chip_font.setPointSizeF(9.5);
    painter.setFont(chip_font);
    const QString label = timeLabel(display_time_);
    const QFontMetricsF fm(chip_font);
    const double chip_w = fm.horizontalAdvance(label) + 20.0;
    const double chip_hi = std::max(6.0, width() - chip_w - 6.0);
    const double chip_x = std::clamp(x - chip_w * 0.5, 6.0, chip_hi);
    const QRectF chip(chip_x, 3.0, chip_w, 19.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 209, 66, 235));
    painter.drawRoundedRect(chip, 7, 7);
    painter.setPen(QColor("#16212d"));
    painter.drawText(chip, Qt::AlignCenter, label);
}

void WeatherTimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    dragging_ = true;
    setTimeFromPosition(event->position().x());
}

void WeatherTimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    hover_x_ = event->position().x();
    hover_active_ = true;
    if (dragging_) {
        setTimeFromPosition(event->position().x());
    } else {
        update();
    }
}

void WeatherTimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    dragging_ = false;
    setTimeFromPosition(event->position().x());
}

void WeatherTimelineWidget::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    hover_active_ = false;
    update();
}

QRectF WeatherTimelineWidget::trackRect() const
{
    return QRectF(rect()).adjusted(54, 30, -54, -34);
}

double WeatherTimelineWidget::maxTimeMin() const
{
    double max_time = 180.0;
    for (const WeatherHazard& hazard : live_hazards_) {
        max_time = std::max(max_time, hazard.valid_to_min);
    }
    for (const LiveWeatherSample& sample : live_samples_) {
        max_time = std::max(max_time, sample.valid_to_min);
    }
    return std::max(30.0, max_time);
}

double WeatherTimelineWidget::timeAtX(double x) const
{
    const QRectF track = trackRect();
    const double ratio = std::clamp((x - track.left()) / std::max(1.0, track.width()), 0.0, 1.0);
    return ratio * maxTimeMin();
}

double WeatherTimelineWidget::xForTime(double minutes) const
{
    const QRectF track = trackRect();
    return track.left() + std::clamp(minutes / maxTimeMin(), 0.0, 1.0) * track.width();
}

void WeatherTimelineWidget::setTimeFromPosition(double x)
{
    forecast_time_min_ = timeAtX(x);
    if (time_changed_) {
        time_changed_(forecast_time_min_);
    }
    update();
}

} // namespace aviwx::gui
