#pragma once

#include "aviwx/Scenario.h"

#include <QColor>
#include <QWidget>

namespace aviwx::gui {

class RouteProfileWidget : public QWidget {
public:
    explicit RouteProfileWidget(QWidget* parent = nullptr);

    void setScenario(const Scenario* scenario);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawEmpty(QPainter& painter);
    void drawProfile(QPainter& painter);
    QColor hazardColor(HazardType type) const;

    const Scenario* scenario_{};
};

} // namespace aviwx::gui
