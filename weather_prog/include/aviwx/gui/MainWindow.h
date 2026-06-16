#pragma once

#include "aviwx/GisMap.h"
#include "aviwx/GdalSxfReader.h"
#include "aviwx/LiveWeather.h"
#include "aviwx/RiskEngine.h"
#include "aviwx/Scenario.h"

#include <QMainWindow>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

class QEvent;
class QLabel;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QNetworkAccessManager;
class QNetworkReply;
class QShowEvent;
class QTextEdit;
class QTimer;
class QWidget;

namespace aviwx::gui {

class ModeBar;
class RouteProfileWidget;
class WeatherTimelineWidget;
class WeatherMapWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

    bool loadScenarioFile(const std::filesystem::path& path);
    bool loadSxfScale(const std::string& scale);
    bool loadSxfFolderPath(const std::filesystem::path& root);
    bool exportScreenshot(const std::filesystem::path& path);

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct WeatherBounds {
        double min_lat{};
        double max_lat{};
        double min_lon{};
        double max_lon{};
    };

    void buildUi();
    void refreshUi();
    void refreshFindings();
    void loadSxfFolder();
    bool loadSxfViewport(double min_lat, double max_lat, double min_lon, double max_lon);
    WeatherBounds initialBoundsForTiles() const;
    void scheduleWeatherRequest(double min_lat, double max_lat, double min_lon, double max_lon);
    void requestWeatherForPendingViewport();
    void handleWeatherReply(QNetworkReply* reply);
    std::filesystem::path builtInSxfPath(const std::string& scale) const;
    void showFindingDetail(int row);
    void showError(const QString& message);
    void addHoverGlow(QWidget* widget);

    std::optional<Scenario> scenario_;
    RiskReport report_;
    std::shared_ptr<const GisMap> base_map_;
    std::filesystem::path current_path_;
    std::filesystem::path current_sxf_folder_;
    std::vector<SxfMapTile> sxf_tiles_;
    std::vector<std::filesystem::path> loaded_sxf_files_;
    bool sxf_loading_{false};
    WeatherBounds pending_weather_bounds_;
    bool has_pending_weather_bounds_{false};
    bool weather_request_running_{false};
    bool first_show_{true};
    std::vector<WeatherHazard> live_weather_;
    std::vector<LiveWeatherSample> live_samples_;

    WeatherMapWidget* map_{};
    ModeBar* mode_bar_{};
    QNetworkAccessManager* weather_network_{};
    QTimer* weather_debounce_timer_{};
    QLabel* route_label_{};
    QLabel* risk_label_{};
    QLabel* source_label_{};
    QComboBox* map_scale_combo_{};
    QListWidget* findings_list_{};
    QTextEdit* finding_details_{};
    RouteProfileWidget* route_profile_{};
    WeatherTimelineWidget* timeline_{};
};

} // namespace aviwx::gui
