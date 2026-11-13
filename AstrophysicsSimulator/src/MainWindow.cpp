#include "MainWindow.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeySequence>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cmath>

namespace {
QLabel* headline(const QString& text) {
    auto* label=new QLabel(text);
    label->setObjectName("sectionTitle");
    return label;
}
QFrame* rule() {
    auto* frame=new QFrame;
    frame->setFrameShape(QFrame::HLine);
    frame->setObjectName("rule");
    return frame;
}
}

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent) {
    setWindowTitle("Astrophysics Simulator • C++ / Qt 6");
    resize(1400,870);
    setMinimumSize(960,650);
    auto* root=new QWidget;
    auto* row=new QHBoxLayout(root);
    row->setContentsMargins(0,0,0,0);row->setSpacing(0);

    auto* panel=new QFrame;
    panel->setObjectName("sidePanel");
    panel->setFixedWidth(322);
    sidebarScroll_=new QScrollArea;
    sidebarScroll_->setWidgetResizable(true);
    sidebarScroll_->setFixedWidth(322);
    sidebarScroll_->setFrameShape(QFrame::NoFrame);
    sidebarScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarScroll_->setWidget(panel);
    row->addWidget(sidebarScroll_);

    auto* sidebar=new QVBoxLayout(panel);
    sidebar->setContentsMargins(23,24,23,23);
    sidebar->setSpacing(12);
    auto* brand=new QLabel("ASTROPHYSICS");brand->setObjectName("brand");sidebar->addWidget(brand);
    auto* subbrand=new QLabel("S I M U L A T O R    /    0 . 8");
    subbrand->setObjectName("subbrand");sidebar->addWidget(subbrand);
    sidebar->addSpacing(20);
    sidebar->addWidget(headline("РЕЖИМ ИССЛЕДОВАНИЯ"));
    const QString names[4]={"01  MILKY WAY","02  BLACK HOLE",
                            "03  GALACTIC CENTER","04  PHYSICS"};
    for(int i=0;i<4;++i) {
        auto* button=new QPushButton(names[i]);
        button->setCheckable(true);button->setObjectName("modeButton");
        button->setMinimumHeight(48);
        sidebar->addWidget(button);
        modeButtons_[i]=button;
    }
    sidebar->addSpacing(8);
    modeDescription_=new QLabel;
    modeDescription_->setWordWrap(true);
    modeDescription_->setObjectName("modeDescription");
    sidebar->addWidget(modeDescription_);
    photonDiagramButton_=new QPushButton("▶  ФОТОННЫЕ ТРАЕКТОРИИ");
    photonDiagramButton_->setObjectName("accentButton");
    photonDiagramButton_->setCheckable(true);
    photonDiagramButton_->setMinimumHeight(42);
    sidebar->addWidget(photonDiagramButton_);
    fullscreenButton_=new QPushButton("⛶  ВО ВЕСЬ ЭКРАН");
    fullscreenButton_->setObjectName("neutralButton");
    fullscreenButton_->setMinimumHeight(38);
    sidebar->addWidget(fullscreenButton_);
    sidebar->addSpacing(8);
    sidebar->addWidget(rule());
    sidebar->addSpacing(7);
    sidebar->addWidget(headline("ВРЕМЯ СИМУЛЯЦИИ"));
    speedLabel_=new QLabel;speedLabel_->setObjectName("valueLabel");
    sidebar->addWidget(speedLabel_);
    speedSlider_=new QSlider(Qt::Horizontal);
    speedSlider_->setRange(0,100);
    speedSlider_->setValue(50);
    sidebar->addWidget(speedSlider_);
    pauseButton_=new QPushButton("⏸  Пауза");
    pauseButton_->setMinimumHeight(38);
    pauseButton_->setObjectName("neutralButton");
    sidebar->addWidget(pauseButton_);
    sidebar->addSpacing(9);
    sidebar->addWidget(rule());
    sidebar->addSpacing(7);
    sidebar->addWidget(headline("ПАРАМЕТРЫ ГАЛАКТИКИ"));
    auto* starLabel=new QLabel("Количество звёзд");sidebar->addWidget(starLabel);
    starCount_=new QComboBox;
    for(int count:{25000,100000,250000,500000,1000000})
        starCount_->addItem(QString::number(count),count);
    starCount_->setCurrentIndex(1);
    sidebar->addWidget(starCount_);
    halo_=new QCheckBox("Гало тёмной материи");
    halo_->setChecked(true);
    sidebar->addWidget(halo_);
    sidebar->addSpacing(10);
    sidebar->addWidget(headline("ЦЕНТРАЛЬНАЯ ЧЁРНАЯ ДЫРА"));
    massLabel_=new QLabel;
    sidebar->addWidget(massLabel_);
    massSlider_=new QSlider(Qt::Horizontal);
    massSlider_->setRange(10,100);massSlider_->setValue(42);
    sidebar->addWidget(massSlider_);
    spinLabel_=new QLabel;
    sidebar->addWidget(spinLabel_);
    spinSlider_=new QSlider(Qt::Horizontal);
    spinSlider_->setRange(-998,998);spinSlider_->setValue(650);
    sidebar->addWidget(spinSlider_);
    physicsGravity_=new QCheckBox("Physics: гравитация объектов");
    physicsGravity_->setChecked(false);
    sidebar->addWidget(physicsGravity_);
    sidebar->addSpacing(9);
    sidebar->addWidget(rule());
    sidebar->addSpacing(7);
    sidebar->addWidget(headline("ФИЗИЧЕСКИЕ ПОКАЗАТЕЛИ"));
    statsLabel_=new QLabel;
    statsLabel_->setWordWrap(true);
    statsLabel_->setObjectName("statsLabel");
    sidebar->addWidget(statsLabel_);
    sidebar->addStretch(1);
    auto* controls=new QLabel("ЛКМ: вращение / наклон диска  •  ПКМ: сдвиг\nPhysics: ЛКМ/СКМ — орбита, удерживать ПКМ — гравитация\nКолесо: масштаб  •  WASD/QE: полёт\n1 / 2 / 3 / 4: режимы  •  Space: пауза\nR: сброс  •  H: тёмное гало  •  G: Physics-гравитация\nEsc: выход из фотонного/полноэкранного режима");
    controls->setObjectName("hintLabel");
    controls->setWordWrap(true);
    sidebar->addWidget(controls);

    view_=new GLView;
    row->addWidget(view_,1);
    setCentralWidget(root);

    for(int i=0;i<4;++i)
        connect(modeButtons_[i],&QPushButton::clicked,this,[this,i]{view_->setMode(static_cast<GLView::Mode>(i));});
    connect(view_,&GLView::modeChanged,this,&MainWindow::syncMode);
    connect(view_,&GLView::statsUpdated,statsLabel_,&QLabel::setText);
    connect(view_,&GLView::pauseChanged,this,[this](bool paused){
        pauseButton_->setText(paused?"▶  Продолжить":"⏸  Пауза");
    });
    connect(pauseButton_,&QPushButton::clicked,this,[this]{view_->setPaused(!view_->paused());});
    connect(speedSlider_,&QSlider::valueChanged,this,[this](int){refreshSpeedLabel();});
    connect(starCount_,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int){
        view_->setStarCount(starCount_->currentData().toInt());
    });
    connect(halo_,&QCheckBox::toggled,view_,&GLView::setHaloEnabled);
    connect(physicsGravity_,&QCheckBox::toggled,view_,&GLView::setPhysicsGravityEnabled);
    connect(view_,&GLView::physicsGravityChanged,physicsGravity_,&QCheckBox::setChecked);
    connect(photonDiagramButton_,&QPushButton::clicked,this,[this]{
        const bool enabled=!view_->photonDiagramEnabled();
        view_->setPhotonDiagramEnabled(enabled);
    });
    connect(view_,&GLView::photonDiagramChanged,this,[this](bool enabled){
        photonDiagramButton_->setChecked(enabled);
        photonDiagramButton_->setText(enabled
            ?"■  ВЕРНУТЬ ОСНОВНУЮ СЦЕНУ":"▶  ФОТОННЫЕ ТРАЕКТОРИИ");
    });
    connect(fullscreenButton_,&QPushButton::clicked,this,[this]{
        setSimulationFullscreen(!simulationFullscreen_);
    });
    auto* escapeShortcut=new QShortcut(QKeySequence(Qt::Key_Escape),this);
    escapeShortcut->setContext(Qt::ApplicationShortcut);
    connect(escapeShortcut,&QShortcut::activated,this,[this]{
        if(simulationFullscreen_) {
            setSimulationFullscreen(false);
            if(view_->photonDiagramEnabled()) view_->setPhotonDiagramEnabled(false);
        }
        else if(view_->mode()==GLView::Mode::Physics && view_->photonDiagramEnabled())
            view_->setPhotonDiagramEnabled(false);
    });
    connect(massSlider_,&QSlider::valueChanged,this,[this](int value){
        const double solar=value*1e5;
        view_->setBlackHoleMass(solar);
        massLabel_->setText(QString("Масса: %1 млн M☉").arg(solar/1e6,0,'f',1));
    });
    connect(spinSlider_,&QSlider::valueChanged,this,[this](int value){
        const double spin=value/1000.0;
        view_->setBlackHoleSpin(spin);
        spinLabel_->setText(QString("Спин Керра a*: %1").arg(spin,0,'f',3));
    });
    massLabel_->setText("Масса: 4.2 млн M☉");
    spinLabel_->setText("Спин Керра a*: 0.650");
    view_->setBlackHoleMass(4.2e6);
    view_->setBlackHoleSpin(0.65);
    syncMode(0);
    refreshSpeedLabel();
    statsLabel_->setText(view_->statistics());
    setStyleSheet(R"QSS(
        QMainWindow, QWidget { background:#070b15; color:#dfecff; font-family: -apple-system, "Helvetica Neue", Arial; font-size:13px; }
        QFrame#sidePanel { background:#101725; border-right:1px solid #2a3850; }
        QScrollArea { background:#101725; border:none; }
        QLabel#brand { color:#e7efff; font-size:21px; font-weight:800; letter-spacing:3px; }
        QLabel#subbrand { color:#5d8cda; font-size:11px; letter-spacing:2px; }
        QLabel#sectionTitle { color:#93aacf; font-size:10px; font-weight:700; letter-spacing:1px; }
        QLabel#modeDescription { color:#879cba; font-size:12px; line-height:1.4; }
        QLabel#valueLabel { color:#e9f2ff; font-size:15px; font-weight:600; }
        QLabel#statsLabel { color:#abc0d8; font-family: Menlo, monospace; font-size:11px; line-height:1.7; }
        QLabel#hintLabel { color:#657790; font-size:11px; line-height:1.5; }
        QFrame#rule { color:#243149; background:#243149; max-height:1px; border:0px; }
        QPushButton#modeButton { background:#171f30; color:#9fb0ca; text-align:left; padding-left:15px; border:1px solid #28354d; border-radius:8px; font-weight:650; }
        QPushButton#modeButton:hover { border-color:#577aab; color:#ffffff; }
        QPushButton#modeButton:checked { background:#172f52; border:1px solid #4b94ec; color:#d8edff; }
        QPushButton#neutralButton { background:#1b273b; border:1px solid #35445f; border-radius:7px; }
        QPushButton#neutralButton:hover { background:#2b3e5d; }
        QPushButton#accentButton { background:#153d67; color:#e6f3ff; border:1px solid #4b9cff; border-radius:7px; font-weight:700; }
        QPushButton#accentButton:hover { background:#20558b; }
        QPushButton#accentButton:checked { background:#64331b; border-color:#ff8a3d; }
        QComboBox { background:#1a2639; color:#e2f0ff; border:1px solid #344561; border-radius:6px; padding:7px; }
        QComboBox QAbstractItemView { background:#1a2639; color:#e2f0ff; }
        QCheckBox { color:#c6d5eb; spacing:9px; }
        QSlider::groove:horizontal { height:4px; border-radius:2px; background:#2d405b; }
        QSlider::sub-page:horizontal { background:#4b96ff; border-radius:2px; }
        QSlider::handle:horizontal { background:#c8e7ff; width:13px; height:13px; margin:-5px 0; border-radius:7px; }
        QScrollBar:vertical { background:#101725; width:6px; }
        QScrollBar::handle:vertical { background:#304157; border-radius:3px; }
    )QSS");
}

void MainWindow::syncMode(int mode) {
    for(int i=0;i<4;++i) modeButtons_[i]->setChecked(i==mode);
    static const QString details[4]={
        "Трёхмерный Млечный Путь. Звёзды движутся в потенциале диска, балджа и гало. Колесом можно приблизиться к центру.",
        "Единая релятивистская сцена: чёрная дыра деформирует сетку, световые лучи следуют нулевым геодезическим, диск и тень накладываются в той же камере.",
        "Около Стрельца A*: подвижная чёрная дыра, N-body, 1PN, увлечение инерциальных систем, TDE и прямой захват.",
        "Адаптированная сцена kavan010/black_hole: квадратная сетка, Schwarzschild ray tracer и встроенные фотонные траектории. Кнопка полного экрана доступна для любой сцены."
    };
    const int safeMode=std::clamp(mode,0,3);
    modeDescription_->setText(details[safeMode]);
    const bool physicsMode=safeMode==int(GLView::Mode::Physics);
    physicsGravity_->setEnabled(physicsMode);
    photonDiagramButton_->setVisible(physicsMode);
    fullscreenButton_->setVisible(true);
    refreshSpeedLabel();
}
void MainWindow::refreshSpeedLabel() {
    const double time=std::pow(10.0,(speedSlider_->value()-50.0)/30.0);
    view_->setTimeScale(time);
    const QString unit=(view_->mode()==GLView::Mode::MilkyWay?"млн лет/с":
                       view_->mode()==GLView::Mode::BlackHole?"× скорость анимации":
                       view_->mode()==GLView::Mode::Physics?"Rs/c в секунду":"лет/с");
    speedLabel_->setText(QString("Скорость: %1 %2").arg(time,0,'f',2).arg(unit));
}

void MainWindow::setSimulationFullscreen(bool enabled) {
    if(simulationFullscreen_==enabled) return;
    simulationFullscreen_=enabled;
    if(enabled) {
        wasMaximizedBeforeFullscreen_=isMaximized();
        sidebarScroll_->hide();
        showFullScreen();
        view_->setFocus();
    } else {
        if(wasMaximizedBeforeFullscreen_) showMaximized();
        else showNormal();
        sidebarScroll_->show();
        view_->setFocus();
    }
}
