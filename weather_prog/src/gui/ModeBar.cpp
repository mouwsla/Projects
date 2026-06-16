#include "aviwx/gui/ModeBar.h"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <utility>

namespace aviwx::gui {
namespace {

QColor modeColor(WeatherMode mode)
{
    switch (mode) {
    case WeatherMode::Precipitation: return QColor("#5d8ee8");
    case WeatherMode::Clouds:        return QColor("#b9c4d2");
    case WeatherMode::Temperature:   return QColor("#f0894c");
    case WeatherMode::Wind:          return QColor("#cfe0ff");
    case WeatherMode::Visibility:    return QColor("#d4cdb4");
    case WeatherMode::Pressure:      return QColor("#e6dcae");
    }
    return QColor("#cfd9e4");
}

QColor lerpColor(const QColor& a, const QColor& b, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return QColor(
        static_cast<int>(std::lround(a.red() + (b.red() - a.red()) * t)),
        static_cast<int>(std::lround(a.green() + (b.green() - a.green()) * t)),
        static_cast<int>(std::lround(a.blue() + (b.blue() - a.blue()) * t)));
}

} // namespace

ModeBar::ModeBar(QWidget* parent)
    : QWidget(parent)
{
    items_ = {
        {"Осадки", WeatherMode::Precipitation},
        {"Облачность", WeatherMode::Clouds},
        {"Температура", WeatherMode::Temperature},
        {"Ветер", WeatherMode::Wind},
        {"Видимость", WeatherMode::Visibility},
        {"Давление", WeatherMode::Pressure},
    };
    hover_glow_.assign(items_.size(), 0.0);

    setMouseTracking(true);
    setFixedHeight(58);
    setMinimumWidth(520);
    setCursor(Qt::PointingHandCursor);

    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, [this] { onTick(); });
    timer_->start();
}

void ModeBar::setModeChangedCallback(std::function<void(WeatherMode)> callback)
{
    mode_changed_ = std::move(callback);
}

void ModeBar::setCurrentMode(WeatherMode mode)
{
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].mode == mode) {
            current_ = static_cast<int>(i);
            update();
            return;
        }
    }
}

WeatherMode ModeBar::currentMode() const
{
    if (current_ >= 0 && current_ < static_cast<int>(items_.size())) {
        return items_[static_cast<std::size_t>(current_)].mode;
    }
    return WeatherMode::Precipitation;
}

QSize ModeBar::sizeHint() const
{
    return QSize(760, 58);
}

QRectF ModeBar::itemRect(double index) const
{
    const double pad = 7.0;
    const double count = std::max<double>(1.0, static_cast<double>(items_.size()));
    const double slot = (width() - 2.0 * pad) / count;
    const double gap = 5.0;
    return QRectF(pad + index * slot + gap * 0.5,
                  9.0,
                  std::max(8.0, slot - gap),
                  height() - 18.0);
}

int ModeBar::itemAt(const QPointF& point) const
{
    if (items_.empty()) {
        return -1;
    }
    const double pad = 7.0;
    const double slot = (width() - 2.0 * pad) / static_cast<double>(items_.size());
    if (slot <= 0.0) {
        return -1;
    }
    const int index = static_cast<int>(std::floor((point.x() - pad) / slot));
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return -1;
    }
    return index;
}

void ModeBar::onTick()
{
    constexpr double dt = 1.0 / 60.0;
    const double k = 1.0 - std::exp(-18.0 * dt);
    bool busy = false;

    const double next = anim_pos_ + (static_cast<double>(current_) - anim_pos_) * k;
    if (std::abs(next - anim_pos_) > 1e-4) {
        anim_pos_ = next;
        busy = true;
    } else if (std::abs(anim_pos_ - current_) > 0.0) {
        anim_pos_ = current_;
        busy = true;
    }

    for (std::size_t i = 0; i < hover_glow_.size(); ++i) {
        const double target = (static_cast<int>(i) == hover_) ? 1.0 : 0.0;
        const double value = hover_glow_[i] + (target - hover_glow_[i]) * k;
        if (std::abs(value - hover_glow_[i]) > 1e-4) {
            hover_glow_[i] = value;
            busy = true;
        } else if (hover_glow_[i] != target) {
            hover_glow_[i] = target;
            busy = true;
        }
    }

    if (press_glow_ > 0.0) {
        press_glow_ = std::max(0.0, press_glow_ - dt / 0.22);
        busy = true;
    }

    if (busy) {
        update();
    }
}

void ModeBar::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#0f1822"));
    painter.setPen(QPen(QColor("#1d2935"), 1));
    painter.drawLine(QPointF(rect().left(), rect().bottom()),
                     QPointF(rect().right(), rect().bottom()));

    if (items_.empty()) {
        return;
    }

    // hover halo behind non-active items
    for (std::size_t i = 0; i < items_.size(); ++i) {
        if (static_cast<int>(i) == current_ || hover_glow_[i] <= 0.01) {
            continue;
        }
        const QRectF r = itemRect(static_cast<double>(i));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, static_cast<int>(22 * hover_glow_[i])));
        painter.drawRoundedRect(r, 13, 13);
    }

    // sliding active pill
    QRectF pill = itemRect(anim_pos_);
    if (press_glow_ > 0.0) {
        const double grow = 2.0 * press_glow_;
        pill = pill.adjusted(-grow, -grow, grow, grow);
    }
    QLinearGradient gradient(pill.topLeft(), pill.bottomLeft());
    gradient.setColorAt(0.0, QColor("#ffe788"));
    gradient.setColorAt(1.0, QColor("#f3b338"));
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(pill, 13, 13);

    QFont font = painter.font();
    font.setPointSizeF(10.5);
    font.setBold(true);
    painter.setFont(font);
    const QFontMetricsF fm(font);

    for (std::size_t i = 0; i < items_.size(); ++i) {
        const QRectF r = itemRect(static_cast<double>(i));
        const QString& label = items_[i].label;
        const double sel = std::clamp(1.0 - std::abs(anim_pos_ - static_cast<double>(i)), 0.0, 1.0);

        QColor text_color;
        if (sel > 0.02) {
            text_color = lerpColor(QColor("#9aa8b8"), QColor("#16212d"), sel);
        } else {
            text_color = lerpColor(QColor("#8b9aab"), QColor("#eef3f9"), hover_glow_[i]);
        }

        const double dot_radius = 3.6;
        const double text_width = fm.horizontalAdvance(label);
        const double gap = 8.0;
        const double group = dot_radius * 2.0 + gap + text_width;
        const double start_x = r.center().x() - group * 0.5;
        const double centre_y = r.center().y();

        QColor dot = modeColor(items_[i].mode);
        if (sel <= 0.02) {
            dot.setAlpha(static_cast<int>(150 + 105 * hover_glow_[i]));
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(dot);
        painter.drawEllipse(QPointF(start_x + dot_radius, centre_y), dot_radius, dot_radius);

        const QRectF text_rect(start_x + dot_radius * 2.0 + gap, r.top(),
                               text_width + 4.0, r.height());
        painter.setPen(text_color);
        painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

void ModeBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const int index = itemAt(event->position());
    if (index < 0) {
        return;
    }
    press_glow_ = 1.0;
    if (index != current_) {
        current_ = index;
        if (mode_changed_) {
            mode_changed_(items_[static_cast<std::size_t>(index)].mode);
        }
    }
    update();
}

void ModeBar::mouseMoveEvent(QMouseEvent* event)
{
    const int index = itemAt(event->position());
    if (index != hover_) {
        hover_ = index;
        update();
    }
}

void ModeBar::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
    hover_ = -1;
    update();
}

} // namespace aviwx::gui
