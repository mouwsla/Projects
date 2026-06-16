#pragma once

#include "aviwx/gui/WeatherMapWidget.h" // WeatherMode

#include <QString>
#include <QWidget>

#include <functional>
#include <vector>

class QTimer;

namespace aviwx::gui {

// Weather-mode selector with a smoothly sliding highlight pill and animated
// hover feedback. Custom-painted so every item animates without relying on
// Qt stylesheets (which cannot tween).
class ModeBar : public QWidget {
public:
    explicit ModeBar(QWidget* parent = nullptr);

    void setModeChangedCallback(std::function<void(WeatherMode)> callback);
    void setCurrentMode(WeatherMode mode);
    WeatherMode currentMode() const;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Item {
        QString label;
        WeatherMode mode;
    };

    QRectF itemRect(double index) const;
    int itemAt(const QPointF& point) const;
    void onTick();

    std::vector<Item> items_;
    std::vector<double> hover_glow_;
    int current_{0};
    int hover_{-1};
    double anim_pos_{0.0};
    double press_glow_{0.0};
    std::function<void(WeatherMode)> mode_changed_;
    QTimer* timer_{};
};

} // namespace aviwx::gui
