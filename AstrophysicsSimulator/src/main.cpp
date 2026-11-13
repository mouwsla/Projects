#include "MainWindow.hpp"
#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>
int main(int argc,char** argv) {
    // macOS supports OpenGL 4.1 core (deprecated by Apple but still available).
    // Use an OpenGL 3.3 feature subset with GLSL 330 core shaders.
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
#ifdef Q_OS_MACOS
    format.setVersion(4,1);
#else
    format.setVersion(3,3);
#endif
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication app(argc,argv);
    QCoreApplication::setApplicationName("Astrophysics Simulator");
    QCoreApplication::setOrganizationName("AstrophysicsSimulator");
    MainWindow window;
    window.show();
    return app.exec();
}
