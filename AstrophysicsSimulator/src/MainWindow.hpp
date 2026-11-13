#pragma once
#include "GLView.hpp"
#include <QMainWindow>
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QSlider;

class MainWindow: public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
private:
    void syncMode(int mode);
    void refreshSpeedLabel();
    void setSimulationFullscreen(bool enabled);
    GLView* view_=nullptr;
    QPushButton* modeButtons_[4]={};
    QPushButton* pauseButton_=nullptr;
    QPushButton* photonDiagramButton_=nullptr;
    QPushButton* fullscreenButton_=nullptr;
    QLabel *statsLabel_=nullptr,*speedLabel_=nullptr,*modeDescription_=nullptr,*massLabel_=nullptr,*spinLabel_=nullptr;
    QSlider* speedSlider_=nullptr;
    QSlider* massSlider_=nullptr;
    QSlider* spinSlider_=nullptr;
    QComboBox* starCount_=nullptr;
    QCheckBox* halo_=nullptr;
    QCheckBox* physicsGravity_=nullptr;
    QScrollArea* sidebarScroll_=nullptr;
    bool simulationFullscreen_=false;
    bool wasMaximizedBeforeFullscreen_=false;
};
