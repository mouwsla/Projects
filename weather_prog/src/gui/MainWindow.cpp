#include "aviwx/gui/MainWindow.h"

#if defined(AVIWX_WITH_GDAL)
#include "aviwx/GdalSxfReader.h"
#endif

#include "aviwx/gui/ModeBar.h"
#include "aviwx/gui/RouteProfileWidget.h"
#include "aviwx/gui/WeatherTimelineWidget.h"
#include "aviwx/gui/WeatherMapWidget.h"

#include <QApplication>
#include <QCheckBox>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QShowEvent>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace aviwx::gui {
namespace {

QString qstr(const std::string& value)
{
    return QString::fromStdString(value);
}

QString riskLevelRu(RiskLevel level)
{
    switch (level) {
    case RiskLevel::Low:
        return "низкий";
    case RiskLevel::Medium:
        return "умеренный";
    case RiskLevel::High:
        return "высокий";
    case RiskLevel::Extreme:
        return "экстремальный";
    }
    return "нет данных";
}

QString riskColorCss(RiskLevel level)
{
    switch (level) {
    case RiskLevel::Low:
        return "#2f8f5b";
    case RiskLevel::Medium:
        return "#b98213";
    case RiskLevel::High:
        return "#bf4d26";
    case RiskLevel::Extreme:
        return "#b52d44";
    }
    return "#596170";
}

QString routeText(const Scenario& scenario)
{
    if (scenario.route.waypoints.empty()) {
        return "Маршрут не загружен";
    }

    const auto& first = scenario.route.waypoints.front();
    const auto& last = scenario.route.waypoints.back();
    return QString("%1 -> %2 | точек: %3 | коридор: %4 NM")
        .arg(qstr(first.name))
        .arg(qstr(last.name))
        .arg(scenario.route.waypoints.size())
        .arg(scenario.route_corridor_nm, 0, 'f', 0);
}

QWidget* separator()
{
    auto* line = new QWidget;
    line->setObjectName("Sep");
    line->setFixedHeight(1);
    return line;
}

QToolButton* makeToolButton(QWidget* parent, const QString& text, QStyle::StandardPixmap icon)
{
    auto* button = new QToolButton(parent);
    button->setText(text);
    button->setIcon(parent->style()->standardIcon(icon));
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return button;
}

std::string hazardIdFromFinding(const RiskFinding& finding)
{
    const std::string marker = " intersects";
    const std::size_t pos = finding.details.find(marker);
    if (pos == std::string::npos) {
        return {};
    }
    return finding.details.substr(0, pos);
}

QString findingTitleRu(const RiskFinding& finding)
{
    const std::string id = hazardIdFromFinding(finding);
    if (!id.empty()) {
        return QString("Погодное явление в зоне маршрута: ") + qstr(id);
    }
    if (finding.title.find("Reduced visibility") != std::string::npos) {
        return "Сниженная видимость на аэродроме";
    }
    if (finding.title.find("Low ceiling") != std::string::npos) {
        return "Низкая облачность на аэродроме";
    }
    if (finding.title.find("Crosswind") != std::string::npos) {
        return "Боковой ветер близок к ограничению";
    }
    if (finding.title.find("No material weather risk") != std::string::npos) {
        return "Существенных рисков не найдено";
    }
    return qstr(finding.title);
}

QString findingDetailsRu(const RiskFinding& finding)
{
    const std::string id = hazardIdFromFinding(finding);
    if (!id.empty()) {
        return "Зона пересекает или подходит к маршруту с учётом времени прогноза, высотного слоя и ширины коридора.";
    }
    if (finding.title.find("Reduced visibility") != std::string::npos) {
        return "Видимость на аэродроме вылета или прилёта ниже авиационного порога.";
    }
    if (finding.title.find("Low ceiling") != std::string::npos) {
        return "Нижняя граница облачности на аэродроме ниже безопасного порога для планирования.";
    }
    if (finding.title.find("Crosswind") != std::string::npos) {
        return "Расчёт боковой составляющей ветра показывает приближение к ограничению воздушного судна.";
    }
    if (finding.title.find("No material weather risk") != std::string::npos) {
        return "Загруженные погодные зоны не пересекают маршрут по времени, высоте и коридору.";
    }
    return "Подробности риска рассчитаны по маршруту, высоте, времени действия явления и коридору.";
}

double jsonNumber(const QJsonObject& object, const QString& key, double fallback = 0.0)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble() : fallback;
}

double hourlyNumber(const QJsonObject& hourly, const QString& key, int index, double fallback = 0.0)
{
    const QJsonArray values = hourly.value(key).toArray();
    if (index < 0 || index >= values.size() || !values[index].isDouble()) {
        return fallback;
    }
    return values[index].toDouble();
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    buildUi();
    refreshUi();
}

bool MainWindow::loadScenarioFile(const std::filesystem::path& path)
{
    try {
        Scenario loaded = load_scenario(path);
        RiskReport report = RiskEngine().analyze(loaded);
        scenario_ = std::move(loaded);
        report_ = std::move(report);
        current_path_ = path;
        refreshUi();
        statusBar()->showMessage("Загружен сценарий: " + QString::fromStdString(path.filename().string()), 3500);
        return true;
    } catch (const std::exception& ex) {
        showError(QString::fromStdString(ex.what()));
        return false;
    }
}

bool MainWindow::exportScreenshot(const std::filesystem::path& path)
{
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    QImage image(size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    render(&painter);
    painter.end();

    const std::filesystem::path out_path = std::filesystem::absolute(path);
    if (!image.save(QString::fromStdString(out_path.string()))) {
        std::cerr << "failed to save screenshot: " << path << "\n";
        return false;
    }
    return true;
}

void MainWindow::buildUi()
{
    setWindowTitle("AviWx - авиационная погода");
    resize(1440, 900);

    weather_network_ = new QNetworkAccessManager(this);
    connect(weather_network_, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
        handleWeatherReply(reply);
    });
    weather_debounce_timer_ = new QTimer(this);
    weather_debounce_timer_->setSingleShot(true);
    connect(weather_debounce_timer_, &QTimer::timeout, this, [this] {
        requestWeatherForPendingViewport();
    });

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    auto* left = new QWidget(splitter);
    left->setMinimumWidth(280);
    left->setMaximumWidth(340);
    auto* left_layout = new QVBoxLayout(left);
    left_layout->setContentsMargins(14, 14, 14, 14);
    left_layout->setSpacing(12);

    auto* title = new QLabel("AviWx");
    title->setObjectName("AppTitle");
    auto* subtitle = new QLabel("Авиационная погодная карта");
    subtitle->setObjectName("MutedText");
    left_layout->addWidget(title);
    left_layout->addWidget(subtitle);

    auto* sxf_scale_button = makeToolButton(left, "Загрузить карту", QStyle::SP_ArrowForward);
    auto* sxf_button = makeToolButton(left, "Папка SXF", QStyle::SP_DirOpenIcon);
    auto* reset_button = makeToolButton(left, "Сбросить вид", QStyle::SP_BrowserReload);
    addHoverGlow(sxf_scale_button);
    addHoverGlow(sxf_button);
    addHoverGlow(reset_button);
    auto* scale_row = new QWidget(left);
    auto* scale_layout = new QHBoxLayout(scale_row);
    scale_layout->setContentsMargins(0, 0, 0, 0);
    scale_layout->setSpacing(8);
    auto* scale_label = new QLabel("Масштаб SXF", scale_row);
    map_scale_combo_ = new QComboBox(scale_row);
    map_scale_combo_->addItem("1M", "1m");
    map_scale_combo_->addItem("200K", "200k");
    map_scale_combo_->setToolTip("Выберите масштаб базовой SXF-карты");
    scale_layout->addWidget(scale_label);
    scale_layout->addWidget(map_scale_combo_, 1);

    connect(sxf_scale_button, &QToolButton::clicked, this, [this] {
        loadSxfScale(map_scale_combo_->currentData().toString().toStdString());
    });
    connect(sxf_button, &QToolButton::clicked, this, [this] {
        loadSxfFolder();
    });
#if !defined(AVIWX_WITH_GDAL)
    sxf_scale_button->setEnabled(false);
    sxf_button->setEnabled(false);
    map_scale_combo_->setEnabled(false);
    sxf_button->setToolTip("GDAL недоступен в этой сборке");
#else
    sxf_scale_button->setToolTip("Загрузить встроенную папку SXF для выбранного масштаба");
    sxf_button->setToolTip("Выбрать папку с SXF-картами");
#endif
    connect(reset_button, &QToolButton::clicked, this, [this] {
        map_->resetView();
    });
    left_layout->addWidget(scale_row);
    left_layout->addWidget(sxf_scale_button);
    left_layout->addWidget(sxf_button);
    left_layout->addWidget(reset_button);
    left_layout->addWidget(separator());

    left_layout->addStretch(1);

    source_label_ = new QLabel;
    source_label_->setObjectName("MutedText");
    source_label_->setWordWrap(true);
    left_layout->addWidget(source_label_);

    auto* center = new QWidget(splitter);
    auto* center_layout = new QVBoxLayout(center);
    center_layout->setContentsMargins(0, 0, 0, 0);
    center_layout->setSpacing(0);

    mode_bar_ = new ModeBar(center);

    map_ = new WeatherMapWidget(center);
    map_->setMinimumSize(620, 520);
    map_->setViewportChangedCallback([this](double min_lat, double max_lat, double min_lon, double max_lon) {
        loadSxfViewport(min_lat, max_lat, min_lon, max_lon);
        scheduleWeatherRequest(min_lat, max_lat, min_lon, max_lon);
    });
    mode_bar_->setModeChangedCallback([this](WeatherMode mode) {
        if (map_) {
            map_->setWeatherMode(mode);
        }
    });

    timeline_ = new WeatherTimelineWidget(center);
    timeline_->setTimeChangedCallback([this](double minutes) {
        map_->setForecastTimeMin(minutes);
    });

    route_profile_ = new RouteProfileWidget(center);
    route_profile_->setVisible(false);
    center_layout->addWidget(mode_bar_);
    center_layout->addWidget(map_, 1);
    center_layout->addWidget(timeline_);
    center_layout->addWidget(route_profile_);

    auto* right = new QWidget(splitter);
    right->setMinimumWidth(330);
    right->setMaximumWidth(420);
    auto* right_layout = new QVBoxLayout(right);
    right_layout->setContentsMargins(14, 14, 14, 14);
    right_layout->setSpacing(12);

    route_label_ = new QLabel;
    route_label_->setObjectName("MutedText");
    route_label_->setWordWrap(true);
    right_layout->addWidget(route_label_);

    risk_label_ = new QLabel;
    risk_label_->setObjectName("RiskLabel");
    risk_label_->setAlignment(Qt::AlignCenter);
    risk_label_->setMinimumHeight(54);
    right_layout->addWidget(risk_label_);

    auto* findings_title = new QLabel("Риски маршрута");
    findings_title->setObjectName("SectionTitle");
    right_layout->addWidget(findings_title);

    findings_list_ = new QListWidget(right);
    findings_list_->setMinimumHeight(260);
    findings_list_->setWordWrap(true);
    findings_list_->setTextElideMode(Qt::ElideRight);
    findings_list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(findings_list_, &QListWidget::currentRowChanged, this, [this](int row) {
        showFindingDetail(row);
    });
    right_layout->addWidget(findings_list_, 1);

    finding_details_ = new QTextEdit(right);
    finding_details_->setReadOnly(true);
    finding_details_->setMinimumHeight(180);
    finding_details_->setObjectName("DetailBox");
    right_layout->addWidget(finding_details_);

    splitter->addWidget(left);
    splitter->addWidget(center);
    splitter->addWidget(right);
    right->hide();
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #0e151c;
            color: #e4eaf1;
            font-size: 13px;
        }
        QLabel#AppTitle {
            font-size: 25px;
            font-weight: 800;
            color: #f4f7fb;
        }
        QLabel#SectionTitle {
            font-size: 12px;
            font-weight: 800;
            color: #8aa0b6;
            margin-top: 6px;
        }
        QLabel#MutedText {
            color: #7d8a9a;
        }
        QLabel#RiskLabel {
            border-radius: 10px;
            color: #ffffff;
            font-size: 20px;
            font-weight: 800;
            padding: 12px;
        }
        QWidget#Sep {
            background: #1f2c38;
        }
        QListWidget, QTextEdit {
            background: #141d26;
            border: 1px solid #243240;
            border-radius: 10px;
            padding: 6px;
            selection-background-color: #1e3a55;
            selection-color: #eaf2fb;
        }
        QListWidget::item {
            padding: 7px 6px;
            border-radius: 7px;
        }
        QListWidget::item:hover {
            background: #1b2935;
        }
        QListWidget::item:selected {
            background: #1e3a55;
        }
        QToolButton {
            background: #18222d;
            border: 1px solid #283643;
            border-radius: 9px;
            padding: 9px 10px;
            color: #dce4ee;
            font-weight: 600;
        }
        QToolButton:hover {
            background: #1f2e3c;
            border-color: #3a6ea5;
            color: #ffffff;
        }
        QToolButton:pressed {
            background: #152029;
        }
        QToolButton:disabled {
            background: #141b22;
            border-color: #1e2832;
            color: #54616f;
        }
        QComboBox {
            background: #18222d;
            border: 1px solid #283643;
            border-radius: 8px;
            padding: 6px 10px;
            color: #e4eaf1;
        }
        QComboBox:hover {
            border-color: #3a6ea5;
        }
        QComboBox::drop-down {
            border: 0;
            width: 18px;
        }
        QComboBox QAbstractItemView {
            background: #141d26;
            border: 1px solid #283643;
            color: #e4eaf1;
            selection-background-color: #1e3a55;
            selection-color: #eaf2fb;
        }
        QCheckBox {
            spacing: 8px;
            color: #cdd9e4;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 5px;
            border: 1px solid #36485a;
            background: #10171f;
        }
        QCheckBox::indicator:hover {
            border-color: #ffd24a;
        }
        QCheckBox::indicator:checked {
            background: #ffd24a;
            border-color: #ffd24a;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #2c3c4c;
            border-radius: 4px;
            min-height: 28px;
        }
        QScrollBar::handle:vertical:hover {
            background: #3a6ea5;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
        }
        QSplitter::handle {
            background: #0e151c;
        }
        QStatusBar {
            background: #0b1117;
            color: #7d8a9a;
        }
        QToolTip {
            background: #10171f;
            color: #e4eaf1;
            border: 1px solid #2c3c4c;
            padding: 5px;
        }
    )");

    setWindowOpacity(0.0);
}

void MainWindow::refreshUi()
{
    if (!scenario_) {
        route_label_->setText("Маршрут не загружен");
        risk_label_->setText("НЕТ ДАННЫХ");
        risk_label_->setStyleSheet("background: #596170;");
        source_label_->setText(base_map_ && !base_map_->empty()
                ? QString("SXF-карта загружена\nЛисты: %1 из %2\nСлои: %3\nОбъекты: %4\nПогода Open-Meteo: %5 точек")
                    .arg(base_map_->sources.size())
                    .arg(sxf_tiles_.empty() ? base_map_->sources.size() : sxf_tiles_.size())
                    .arg(base_map_->layers.size())
                    .arg(base_map_->feature_count())
                    .arg(live_samples_.size())
                : "Откройте SXF-карту: выберите масштаб 1M/200K или папку с листами.");
        map_->setScenario(nullptr, nullptr);
        map_->setBaseMap(base_map_, false);
        timeline_->setScenario(nullptr);
        timeline_->setLiveWeather(live_weather_, live_samples_);
        route_profile_->setScenario(nullptr);
        route_profile_->setVisible(false);
        findings_list_->clear();
        finding_details_->clear();
        return;
    }

    route_label_->setText(routeText(*scenario_));
    risk_label_->setText(QString("%1  %2/100")
        .arg(riskLevelRu(report_.level).toUpper())
        .arg(report_.score, 0, 'f', 1));
    risk_label_->setStyleSheet(QString("background: %1;").arg(riskColorCss(report_.level)));

    QString source = QString("Погода: Open-Meteo\nРежим: live forecast\nОснова: %1")
        .arg(qstr(scenario_->base_map.type));
    if (!scenario_->base_map.path.empty()) {
        source += "\nSXF: " + qstr(scenario_->base_map.path);
    }
    if (base_map_ && !base_map_->empty()) {
        source += QString("\nSXF: %1 лист(ов), %2 слой(ев), %3 объект(ов)\nПогода Open-Meteo: %4 точек")
            .arg(base_map_->sources.size())
            .arg(base_map_->layers.size())
            .arg(base_map_->feature_count())
            .arg(live_samples_.size());
    }
    source_label_->setText(source);

    map_->setScenario(&*scenario_, &report_);
    map_->setBaseMap(base_map_, false);
    timeline_->setScenario(nullptr);
    timeline_->setLiveWeather(live_weather_, live_samples_);
    route_profile_->setScenario(&*scenario_);
    route_profile_->setVisible(false);
    refreshFindings();
}

void MainWindow::refreshFindings()
{
    findings_list_->clear();
    finding_details_->clear();

    for (const RiskFinding& finding : report_.findings) {
        const QString line = QString("[%1] %2\n%3/100")
            .arg(riskLevelRu(finding.level))
            .arg(findingTitleRu(finding))
            .arg(finding.score, 0, 'f', 1);
        auto* item = new QListWidgetItem(line);
        item->setData(Qt::UserRole, QString::fromStdString(hazardIdFromFinding(finding)));
        findings_list_->addItem(item);
    }

    if (findings_list_->count() > 0) {
        findings_list_->setCurrentRow(0);
    }
}

void MainWindow::loadSxfFolder()
{
#if defined(AVIWX_WITH_GDAL)
    const QString folder = QFileDialog::getExistingDirectory(
        this,
        "Открыть папку SXF",
        current_sxf_folder_.empty() ? QString() : QString::fromStdString(current_sxf_folder_.string()));
    if (folder.isEmpty()) {
        return;
    }

    try {
        loadSxfFolderPath(folder.toStdString());
    } catch (const std::exception& ex) {
        showError(QString::fromStdString(ex.what()));
    }
#else
    showError("Эта сборка собрана без поддержки GDAL/SXF.");
#endif
}

bool MainWindow::loadSxfScale(const std::string& scale)
{
    const std::filesystem::path path = builtInSxfPath(scale);
    if (path.empty()) {
        showError("Неизвестный масштаб SXF. Используйте 1m или 200k.");
        return false;
    }
    if (map_scale_combo_) {
        const QString normalized = (scale == "200k" || scale == "200K" || scale == "200000") ? "200k" : "1m";
        const int index = map_scale_combo_->findData(normalized);
        if (index >= 0) {
            map_scale_combo_->setCurrentIndex(index);
        }
    }
    return loadSxfFolderPath(path);
}

bool MainWindow::loadSxfFolderPath(const std::filesystem::path& root)
{
#if defined(AVIWX_WITH_GDAL)
    try {
        std::vector<std::filesystem::path> files = find_sxf_files(root);
        if (files.empty()) {
            showError("В папке не найдены .sxf файлы: " + QString::fromStdString(root.string()));
            return false;
        }

        sxf_tiles_ = index_sxf_files(files);
        if (sxf_tiles_.empty()) {
            showError("GDAL открыл SXF, но не смог построить границы листов карты.");
            return false;
        }

        current_sxf_folder_ = root;
        statusBar()->showMessage(QString("Загружаю все листы SXF (%1)...").arg(files.size()));
        QApplication::processEvents();

        // Load every sheet in the folder in a single pass so the whole map is
        // available. The cap is only a safety ceiling: the bundled 200K folder
        // is ~1.3M objects, so 4M guarantees every sheet is fully loaded.
        auto loaded = std::make_shared<GisMap>(load_sxf_maps(files, 4000000));
        if (loaded->empty()) {
            showError("GDAL открыл SXF, но не нашёл геометрию для отрисовки.");
            return false;
        }

        loaded_sxf_files_ = files;
        base_map_ = std::move(loaded);
        map_->setBaseMap(base_map_, true);
        refreshUi();
        statusBar()->showMessage(QString("Загружено листов SXF: %1 из %2, слоёв: %3, объектов: %4")
                .arg(base_map_->sources.size())
                .arg(files.size())
                .arg(base_map_->layers.size())
                .arg(base_map_->feature_count()),
            6000);
        return true;
    } catch (const std::exception& ex) {
        showError(QString::fromStdString(ex.what()));
        return false;
    }
#else
    (void)root;
    showError("Эта сборка собрана без поддержки GDAL/SXF.");
    return false;
#endif
}

bool MainWindow::loadSxfViewport(double min_lat, double max_lat, double min_lon, double max_lon)
{
#if defined(AVIWX_WITH_GDAL)
    // The whole folder is loaded in one pass by loadSxfFolderPath, so there is
    // no per-viewport streaming left to do once a base map exists.
    if (base_map_ && !base_map_->empty()) {
        return false;
    }
    if (sxf_loading_ || sxf_tiles_.empty()) {
        return false;
    }

    const std::vector<std::filesystem::path> selected = select_sxf_files(
        sxf_tiles_, min_lat, max_lat, min_lon, max_lon, sxf_tiles_.size());
    if (selected.empty() || selected == loaded_sxf_files_) {
        return false;
    }

    sxf_loading_ = true;
    try {
        const bool first_load = !base_map_ || base_map_->empty();
        auto loaded = std::make_shared<GisMap>(load_sxf_maps(selected, 1000000));
        if (loaded->empty()) {
            sxf_loading_ = false;
            showError("GDAL открыл SXF, но не нашёл геометрию для отрисовки.");
            return false;
        }

        loaded_sxf_files_ = selected;
        base_map_ = std::move(loaded);
        map_->setBaseMap(base_map_, first_load);
        refreshUi();
        statusBar()->showMessage(QString("Загружено листов SXF: %1 из %2, слоёв: %3, объектов: %4")
                .arg(base_map_->sources.size())
                .arg(sxf_tiles_.size())
                .arg(base_map_->layers.size())
                .arg(base_map_->feature_count()),
            3500);
        sxf_loading_ = false;
        return true;
    } catch (const std::exception& ex) {
        sxf_loading_ = false;
        showError(QString::fromStdString(ex.what()));
        return false;
    }
#else
    (void)min_lat;
    (void)max_lat;
    (void)min_lon;
    (void)max_lon;
    return false;
#endif
}

MainWindow::WeatherBounds MainWindow::initialBoundsForTiles() const
{
    if (sxf_tiles_.empty()) {
        return WeatherBounds{55.2, 57.2, 36.8, 38.8};
    }

    double min_lat = sxf_tiles_.front().min_lat;
    double max_lat = sxf_tiles_.front().max_lat;
    double min_lon = sxf_tiles_.front().min_lon;
    double max_lon = sxf_tiles_.front().max_lon;
    for (const SxfMapTile& tile : sxf_tiles_) {
        min_lat = std::min(min_lat, tile.min_lat);
        max_lat = std::max(max_lat, tile.max_lat);
        min_lon = std::min(min_lon, tile.min_lon);
        max_lon = std::max(max_lon, tile.max_lon);
    }

    const double center_lat = (min_lat + max_lat) * 0.5;
    const double center_lon = (min_lon + max_lon) * 0.5;
    const double full_lat_span = std::max(0.4, max_lat - min_lat);
    const double full_lon_span = std::max(0.4, max_lon - min_lon);

    const double lat_span = std::clamp(full_lat_span * 0.42, 1.1, 3.2);
    const double lon_span = std::clamp(full_lon_span * 0.42, 1.4, 4.4);
    return WeatherBounds{
        center_lat - lat_span * 0.5,
        center_lat + lat_span * 0.5,
        center_lon - lon_span * 0.5,
        center_lon + lon_span * 0.5,
    };
}

void MainWindow::scheduleWeatherRequest(double min_lat, double max_lat, double min_lon, double max_lon)
{
    if (!weather_debounce_timer_ || !base_map_ || base_map_->empty()) {
        return;
    }

    pending_weather_bounds_ = WeatherBounds{min_lat, max_lat, min_lon, max_lon};
    has_pending_weather_bounds_ = true;
    weather_debounce_timer_->start(700);
}

void MainWindow::requestWeatherForPendingViewport()
{
    if (!weather_network_ || !has_pending_weather_bounds_) {
        return;
    }
    if (weather_request_running_) {
        weather_debounce_timer_->start(900);
        return;
    }

    const WeatherBounds bounds = pending_weather_bounds_;
    has_pending_weather_bounds_ = false;

    const double lat_span = std::max(0.15, bounds.max_lat - bounds.min_lat);
    const double lon_span = std::max(0.15, bounds.max_lon - bounds.min_lon);
    const std::array<double, 4> lats{
        bounds.min_lat + lat_span * 0.16,
        bounds.min_lat + lat_span * 0.39,
        bounds.min_lat + lat_span * 0.62,
        bounds.min_lat + lat_span * 0.85,
    };
    const std::array<double, 4> lons{
        bounds.min_lon + lon_span * 0.16,
        bounds.min_lon + lon_span * 0.39,
        bounds.min_lon + lon_span * 0.62,
        bounds.min_lon + lon_span * 0.85,
    };

    QStringList lat_values;
    QStringList lon_values;
    for (double lat : lats) {
        for (double lon : lons) {
            lat_values << QString::number(std::clamp(lat, -89.9, 89.9), 'f', 5);
            lon_values << QString::number(std::clamp(lon, -179.9, 179.9), 'f', 5);
        }
    }

    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery query;
    query.addQueryItem("latitude", lat_values.join(","));
    query.addQueryItem("longitude", lon_values.join(","));
    query.addQueryItem("hourly", "temperature_2m,precipitation,cloud_cover,visibility,wind_speed_10m,wind_direction_10m,wind_gusts_10m,pressure_msl,cape");
    query.addQueryItem("forecast_days", "1");
    query.addQueryItem("timezone", "UTC");
    query.addQueryItem("wind_speed_unit", "ms");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "AviWx/0.1 aviation-weather-map");
    weather_request_running_ = true;
    weather_network_->get(request);
    statusBar()->showMessage("Запрашиваю реальную погоду Open-Meteo...", 2200);
}

void MainWindow::handleWeatherReply(QNetworkReply* reply)
{
    weather_request_running_ = false;
    const std::unique_ptr<QNetworkReply, void (*)(QNetworkReply*)> cleanup(reply, [](QNetworkReply* owned_reply) {
        if (owned_reply) {
            owned_reply->deleteLater();
        }
    });

    if (!reply) {
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        statusBar()->showMessage("Не удалось получить погоду: " + reply->errorString(), 5000);
        return;
    }

    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError) {
        statusBar()->showMessage("Open-Meteo вернул непонятный JSON: " + parse_error.errorString(), 5000);
        return;
    }

    QJsonArray locations;
    if (document.isArray()) {
        locations = document.array();
    } else if (document.isObject()) {
        locations.append(document.object());
    }

    std::vector<LiveWeatherSample> samples;
    samples.reserve(200);

    for (const QJsonValue& location_value : locations) {
        if (!location_value.isObject()) {
            continue;
        }
        const QJsonObject location = location_value.toObject();
        const double lat = jsonNumber(location, "latitude");
        const double lon = jsonNumber(location, "longitude");
        const QJsonObject hourly = location.value("hourly").toObject();
        const QJsonArray times = hourly.value("time").toArray();
        const QString current_utc_hour = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:00");
        int first_hour = 0;
        for (int i = 0; i < times.size(); ++i) {
            if (times[i].toString() >= current_utc_hour) {
                first_hour = i;
                break;
            }
        }
        const int hours = std::min(12, static_cast<int>(times.size()) - first_hour);

        for (int hour = 0; hour < hours; ++hour) {
            const int source_hour = first_hour + hour;
            LiveWeatherSample sample;
            sample.position = {lat, lon};
            sample.valid_from_min = hour * 60.0;
            sample.valid_to_min = (hour + 1) * 60.0;
            sample.temperature_c = hourlyNumber(hourly, "temperature_2m", source_hour);
            sample.precipitation_mm = hourlyNumber(hourly, "precipitation", source_hour);
            sample.cloud_cover_pct = hourlyNumber(hourly, "cloud_cover", source_hour);
            sample.visibility_m = hourlyNumber(hourly, "visibility", source_hour, -1.0);
            sample.wind_speed_ms = hourlyNumber(hourly, "wind_speed_10m", source_hour);
            sample.wind_gust_ms = hourlyNumber(hourly, "wind_gusts_10m", source_hour);
            sample.wind_direction_deg = hourlyNumber(hourly, "wind_direction_10m", source_hour);
            sample.pressure_hpa = hourlyNumber(hourly, "pressure_msl", source_hour);
            sample.cape_jkg = hourlyNumber(hourly, "cape", source_hour);
            samples.push_back(sample);
        }
    }

    live_samples_ = std::move(samples);
    map_->setLiveWeather(live_weather_, live_samples_);
    timeline_->setLiveWeather(live_weather_, live_samples_);
    refreshUi();
    statusBar()->showMessage(
        QString("Open-Meteo: %1 точек прогноза").arg(live_samples_.size()), 5000);

    if (has_pending_weather_bounds_ && weather_debounce_timer_) {
        weather_debounce_timer_->start(700);
    }
}

std::filesystem::path MainWindow::builtInSxfPath(const std::string& scale) const
{
    const std::filesystem::path cwd = std::filesystem::current_path();
    if (scale == "1m" || scale == "1M" || scale == "1000000") {
        return cwd / "Maps_1M";
    }
    if (scale == "200k" || scale == "200K" || scale == "200000") {
        return cwd / "Maps";
    }
    return {};
}

void MainWindow::showFindingDetail(int row)
{
    if (row < 0 || row >= static_cast<int>(report_.findings.size())) {
        finding_details_->clear();
        return;
    }

    const RiskFinding& finding = report_.findings[static_cast<std::size_t>(row)];
    finding_details_->setPlainText(QString("%1\n\nРиск: %2 (%3/100)\n\n%4")
        .arg(findingTitleRu(finding))
        .arg(riskLevelRu(finding.level))
        .arg(finding.score, 0, 'f', 1)
        .arg(findingDetailsRu(finding)));

    const QString hazard_id = findings_list_->item(row)->data(Qt::UserRole).toString();
    map_->setSelectedHazardId(hazard_id.toStdString());
}

void MainWindow::showError(const QString& message)
{
    QMessageBox::critical(this, "AviWx", message);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (!first_show_) {
        return;
    }
    first_show_ = false;

    auto* fade = new QPropertyAnimation(this, "windowOpacity", this);
    fade->setDuration(360);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    fade->start(QAbstractAnimation::DeleteWhenStopped);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            if (auto* glow = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect())) {
                const bool entering = event->type() == QEvent::Enter;
                auto* anim = new QPropertyAnimation(glow, "blurRadius", glow);
                anim->setDuration(170);
                anim->setStartValue(glow->blurRadius());
                anim->setEndValue(entering ? 24.0 : 0.0);
                anim->setEasingCurve(QEasingCurve::OutCubic);
                anim->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::addHoverGlow(QWidget* widget)
{
    if (!widget) {
        return;
    }
    auto* glow = new QGraphicsDropShadowEffect(widget);
    glow->setOffset(0.0, 0.0);
    glow->setBlurRadius(0.0);
    glow->setColor(QColor(80, 150, 235, 205));
    widget->setGraphicsEffect(glow);
    widget->installEventFilter(this);
}

} // namespace aviwx::gui
