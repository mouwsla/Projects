#pragma once
#include "Physics.hpp"
#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QPoint>
#include <QVector3D>
#include <QTimer>
#include <vector>

class GLView : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
public:
    enum class Mode { MilkyWay=0, BlackHole=1, GalacticCenter=2, Physics=3 };
    explicit GLView(QWidget* parent=nullptr);
    ~GLView() override;
    void setMode(Mode mode);
    Mode mode() const { return mode_; }
    void setPaused(bool paused);
    bool paused() const { return paused_; }
    void setTimeScale(double scale) { timeScale_=scale; }
    void setHaloEnabled(bool enabled);
    bool haloEnabled() const { return galaxy_.params().darkMatter; }
    void setStarCount(int count);
    void setBlackHoleMass(double massSolar);
    void setBlackHoleSpin(double dimensionlessSpin);
    void setPhysicsGravityEnabled(bool enabled);
    bool physicsGravityEnabled() const { return physicsLab_.gravityEnabled(); }
    void setPhotonDiagramEnabled(bool enabled);
    bool photonDiagramEnabled() const { return photonDiagramEnabled_; }
    void resetCurrent();
    void resetCamera();
    int starCount() const { return int(galaxy_.stars().size()); }
    double blackHoleMass() const { return center_.blackHoleMassSolar(); }
    QString statistics() const;

signals:
    void modeChanged(int mode);
    void pauseChanged(bool paused);
    void statsUpdated(const QString& stats);
    void physicsGravityChanged(bool enabled);
    void photonDiagramChanged(bool enabled);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w,int h) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct Vertex { float x,y,z,r,g,b,size; };
    void tick();
    QMatrix4x4 cameraMatrix() const;
    QMatrix4x4 physicsCameraMatrix() const;
    QMatrix4x4 photonDiagramMatrix() const;
    void createPrograms();
    void drawGalaxy();
    void drawCenter();
    void drawBlackHole();
    void drawBlackHoleOverlay();
    void drawSpacetime(bool physicsLab=false);
    void drawPhysics();
    void drawPhysicsGrid();
    void renderPhysicsRayTexture();
    void drawPhotonDiagram();
    void uploadVertices(const std::vector<Vertex>& vertices);
    void rebuildCenterOrbitBuffer();
    void updateKerrShadowTexture();
    void buildSpacetimeGeometry();
    void buildPhotonDiagramGeometry();
    double blackHoleInclination() const;

    Mode mode_=Mode::MilkyWay;
    astro::GalaxySimulation galaxy_;
    astro::GalacticCenterSimulation center_;
    astro::PhysicsLabSimulation physicsLab_;
    std::vector<Vertex> galaxyVertices_, centerVertices_,backdrop_;
    std::vector<float> centerOrbitVertices_;
    std::vector<float> spacetimeVertices_;
    std::vector<float> photonDiagramVertices_;
    std::vector<int> centerOrbitOffsets_,centerOrbitCounts_;
    bool paused_=false,glReady_=false,galaxyDirty_=true,centerOrbitsDirty_=true,kerrLutDirty_=true;
    bool photonDiagramEnabled_=false;
    double timeScale_=1.0,shaderTime_=0.0;
    float yaw_=-70.f,pitch_=37.f,distance_=75.f;
    float blackHoleScale_=14.f;
    QPoint lastMouse_;
    QVector3D cameraTarget_={0,0,0};
    QElapsedTimer clock_;
    QTimer timer_;
    int frameCount_=0;
    int spacetimeSurfaceCount_=0,spacetimeGridFirst_=0,spacetimeGridCount_=0;
    int spacetimeHorizonFirst_=0,spacetimeHorizonCount_=0;
    std::vector<int> photonPathOffsets_,photonPathCounts_;
    std::vector<int> diagramRayOffsets_,diagramRayCounts_;
    int diagramHorizonFirst_=0,diagramHorizonCount_=0;
    int diagramPhotonSphereFirst_=0,diagramPhotonSphereCount_=0;
    int diagramCriticalFirst_=0,diagramCriticalCount_=0;

    std::size_t starVboCapacityBytes_=0;
    GLuint starsVAO_=0,starsVBO_=0,linesVAO_=0,linesVBO_=0,quadVAO_=0,quadVBO_=0;
    GLuint spacetimeVAO_=0,spacetimeVBO_=0;
    GLuint physicsGridVAO_=0,physicsGridVBO_=0;
    GLuint photonDiagramVAO_=0,photonDiagramVBO_=0;
    GLuint physicsFbo_=0,physicsColorTex_=0;
    int physicsGridVertexCount_=0;
    GLuint lensTex_=0,kerrShadowTex_=0;
    QOpenGLShaderProgram starProgram_,lineProgram_,blackHoleProgram_,spacetimeProgram_;
    QOpenGLShaderProgram physicsGridProgram_,physicsRayProgram_,physicsTextureProgram_;
};
