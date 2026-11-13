#include "GLView.hpp"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QVector4D>
#include <QWheelEvent>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>

namespace {
constexpr auto starVertex = R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aColor;
layout(location=2) in float aSize;
uniform mat4 uMVP;
uniform float uPointScale;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPosition,1.0);
    gl_PointSize = clamp(aSize*uPointScale,1.0,32.0);
    vColor=aColor;
}
)GLSL";
constexpr auto starFragment=R"GLSL(#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() {
    float d=length(gl_PointCoord-vec2(0.5))*2.0;
    if(d>1.0) discard;
    float core=pow(max(0.0,1.0-d*d),2.0);
    float glow=0.16*exp(-6.0*d*d);
    fragColor=vec4(vColor*(0.2+1.5*core+glow), min(1.0,core+glow));
}
)GLSL";
constexpr auto lineVertex=R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uMVP;
void main() { gl_Position=uMVP*vec4(aPosition,1.0); }
)GLSL";
constexpr auto lineFragment=R"GLSL(#version 330 core
uniform vec3 uColor;
out vec4 fragColor;
void main() { fragColor=vec4(uColor,0.37); }
)GLSL";
constexpr auto spacetimeVertex=R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uMVP;
uniform float uSpin;
uniform float uTime;
uniform int uBodyCount;
uniform vec4 uBody0;
uniform vec4 uBody1;
uniform vec4 uBody2;
float localWell(vec2 p,vec4 body) {
    vec2 d=p-body.xy;
    return body.w*inversesqrt(dot(d,d)+0.16);
}
void main() {
    float rRs=max(length(aPosition.xy),1.001);
    float r=2.0*rRs; // Boyer-Lindquist radius in GM/c^2
    float a=uSpin;
    float delta=r*r-2.0*r+a*a;
    float denom=(r*r+a*a)*(r*r+a*a)-a*a*delta;
    float omega=(abs(denom)>1e-6 ? 2.0*a*r/denom : 0.0);
    // Rotation visualizes the ZAMO frame-dragging direction. The surface itself
    // remains the exact Schwarzschild equatorial spatial embedding.
    float angle=omega*uTime*1.4;
    float c=cos(angle),s=sin(angle);
    vec2 xy=mat2(c,-s,s,c)*aPosition.xy;
    float extra=0.0;
    if(uBodyCount>0) extra+=localWell(xy,uBody0);
    if(uBodyCount>1) extra+=localWell(xy,uBody1);
    if(uBodyCount>2) extra+=localWell(xy,uBody2);
    gl_Position=uMVP*vec4(xy,aPosition.z-extra,1.0);
}
)GLSL";
constexpr auto spacetimeFragment=R"GLSL(#version 330 core
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor=uColor; }
)GLSL";
constexpr auto physicsGridVertex=R"GLSL(#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uMVP;
uniform vec4 uObject0; // x, z, visible radius, Schwarzschild-radius ratio
uniform vec4 uObject1;
const float referenceOffset=2.364;
float embeddedHeight(vec2 p,vec2 center,float rs) {
    float d=length(p-center);
    if(d>rs) return 2.0*sqrt(max(0.0,rs*(d-rs)))-referenceOffset;
    return 2.0*rs-referenceOffset;
}
void main() {
    vec2 p=aPosition.xz;
    float y=embeddedHeight(p,vec2(0.0),1.0)
           +embeddedHeight(p,uObject0.xy,max(uObject0.w,1e-8))
           +embeddedHeight(p,uObject1.xy,max(uObject1.w,1e-8));
    gl_Position=uMVP*vec4(aPosition.x,y,aPosition.z,1.0);
}
)GLSL";
constexpr auto physicsGridFragment=R"GLSL(#version 330 core
out vec4 fragColor;
void main() { fragColor=vec4(0.50,0.50,0.50,0.70); }
)GLSL";
constexpr auto quadVertex=R"GLSL(#version 330 core
layout(location=0) in vec2 aPosition;
out vec2 vUV;
void main() { vUV=(aPosition+1.0)*0.5; gl_Position=vec4(aPosition,0.0,1.0); }
)GLSL";
constexpr auto physicsTextureFragment=R"GLSL(#version 330 core
in vec2 vUV;
uniform sampler2D uImage;
out vec4 fragColor;
void main() { fragColor=texture(uImage,vUV); }
)GLSL";
constexpr auto physicsRayFragment=R"GLSL(#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform vec3 uCameraPos;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uCameraForward;
uniform vec4 uObject0; // xyz=center, w=radius
uniform vec4 uObject1;
const float pi=3.14159265358979323846;

bool sphereHit(vec3 p,vec4 objectData) {
    return distance(p,objectData.xyz)<=objectData.w;
}

void main() {
    vec2 screen=vUV*2.0-1.0;
    screen.x*=4.0/3.0;
    vec3 direction=normalize(uCameraForward
                    +screen.x*0.577350269*uCameraRight
                    +screen.y*0.577350269*uCameraUp);
    vec3 position=uCameraPos;
    float radius=length(position);
    float theta=acos(clamp(position.z/radius,-0.999999,0.999999));
    float phi=atan(position.y,position.x);
    float st=max(abs(sin(theta)),1e-5);
    float ct=cos(theta);
    vec3 er=vec3(st*cos(phi),st*sin(phi),ct);
    vec3 et=vec3(ct*cos(phi),ct*sin(phi),-st);
    vec3 ep=vec3(-sin(phi),cos(phi),0.0);
    float dr=dot(direction,er);
    float dtheta=dot(direction,et)/radius;
    float dphi=dot(direction,ep)/(radius*st);
    float metric=max(1.0-1.0/radius,1e-4);
    float energy=metric*sqrt(max(0.0,dr*dr/metric
                        +radius*radius*(dtheta*dtheta+st*st*dphi*dphi)));
    bool hitHole=false,hitDisk=false;
    int hitObject=0;
    float diskRadius=0.0;
    vec3 hitPosition=vec3(0.0);

    for(int step=0;step<1100;++step) {
        if(radius<=1.001) { hitHole=true; break; }
        vec3 previous=position;
        st=max(abs(sin(theta)),1e-5);
        ct=cos(theta);
        metric=max(1.0-1.0/radius,1e-4);
        float dt=energy/metric;
        float ar=-(0.5/(radius*radius))*metric*dt*dt
                 +(0.5/(radius*radius*metric))*dr*dr
                 +radius*(dtheta*dtheta+st*st*dphi*dphi);
        float at=-2.0*dr*dtheta/radius+st*ct*dphi*dphi;
        float ap=-2.0*dr*dphi/radius-2.0*ct*dtheta*dphi/st;
        float h=clamp(0.009*radius,0.003,0.070);
        radius+=h*dr;
        theta=clamp(theta+h*dtheta,0.0001,pi-0.0001);
        phi+=h*dphi;
        dr+=h*ar;
        dtheta+=h*at;
        dphi+=h*ap;
        position=radius*vec3(sin(theta)*cos(phi),sin(theta)*sin(phi),cos(theta));

        float planeRadius=length(position.xz);
        if(previous.y*position.y<0.0 && planeRadius>=2.2 && planeRadius<=5.2) {
            hitDisk=true;diskRadius=planeRadius;break;
        }
        if(sphereHit(position,uObject0)) { hitObject=1;hitPosition=position;break; }
        if(sphereHit(position,uObject1)) { hitObject=2;hitPosition=position;break; }
        if(radius>55.0 && step>20) break;
    }

    if(hitDisk) {
        float radialColor=clamp(diskRadius/5.2,0.0,1.0);
        fragColor=vec4(1.0,radialColor,0.20,radialColor);
    } else if(hitHole) {
        fragColor=vec4(0.0,0.0,0.0,1.0);
    } else if(hitObject>0) {
        vec4 objectData=(hitObject==1?uObject0:uObject1);
        vec3 base=(hitObject==1?vec3(1.0,1.0,0.0):vec3(1.0,0.0,0.0));
        vec3 normal=normalize(hitPosition-objectData.xyz);
        vec3 viewDirection=normalize(uCameraPos-hitPosition);
        float light=0.10+0.90*max(dot(normal,viewDirection),0.0);
        fragColor=vec4(base*light,1.0);
    } else {
        fragColor=vec4(0.0);
    }
}
)GLSL";
constexpr auto blackHoleFragment=R"GLSL(#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler1D uLens;
uniform sampler1D uShadow;
uniform float uAspect;
uniform float uScale;
uniform float uTime;
uniform float uYaw;
uniform float uPitch;
uniform float uSpin;
uniform float uInclination;
uniform float uIsco;
const float critical=2.598076211;
const float tau=6.283185307;
float h21(vec2 p){ p=fract(p*vec2(123.34,456.21)); p+=dot(p,p+45.32); return fract(p.x*p.y); }
float noise(vec2 p) {
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    return mix(mix(h21(i),h21(i+vec2(1,0)),f.x),
               mix(h21(i+vec2(0,1)),h21(i+vec2(1,1)),f.x),f.y);
}
vec3 background(vec2 p) {
    vec2 q=p*1.9+vec2(uYaw*0.005,uPitch*0.005);
    float cloud=noise(q*3.0)*0.13+noise(q*8.0)*0.06;
    vec3 col=vec3(0.002,0.004,0.012)+cloud*vec3(0.07,0.085,0.13);
    for(int k=0;k<2;++k) {
        vec2 st=(p+vec2(float(k)*1.87))*float(k==0?135:290);
        vec2 cell=floor(st);
        float pick=h21(cell+float(k)*13.0);
        vec2 star=vec2(h21(cell+7.1),h21(cell+27.2));
        float dist=length(fract(st)-star);
        float mask=step(k==0?0.987:0.998,pick);
        float shine=mask*exp(-dist*dist*(k==0?600.0:1800.0));
        col+=shine*(k==0?vec3(1.0,0.87,0.72):vec3(0.7,0.83,1.0));
    }
    return col;
}
void main() {
    vec2 p=vUV*2.0-1.0;
    p.x*=uAspect;
    float b=length(p)*uScale;
    vec2 radial=normalize(p+vec2(1e-10));
    float shadowAngle=fract(atan(p.y,p.x)/tau+1.0);
    float shadowBoundary=texture(uShadow,shadowAngle).r;
    float alpha=0.0;
    if(b>shadowBoundary) {
        float lensB=max(b,critical+0.001);
        float t=clamp((lensB-critical-0.001)/(60.0-critical-0.001),0.0,1.0);
        alpha=texture(uLens,t).r;
        if(b>60.0) alpha=2.0/b;
    }
    // The radial term uses numerically integrated Schwarzschild deflection. The
    // smaller tangential term is the leading spin-dependent frame dragging.
    vec2 tangent=vec2(-radial.y,radial.x);
    vec2 warp=p+radial*(0.060*min(alpha,16.0))
                +tangent*(0.025*uSpin*sin(uInclination)/(b*b+0.35));
    vec3 col=background(warp);

    // Optically thin, zero-torque disk approximation. Its inner edge is the
    // exact equatorial Kerr ISCO; g^3 supplies relativistic beaming/redshift.
    float axisRatio=max(0.075,abs(cos(uInclination)));
    float diskR=length(vec2(p.x,p.y/axisRatio))*uScale;
    float diskRing=smoothstep(uIsco,uIsco+0.45,diskR)
                  *(1.0-smoothstep(22.0,32.0,diskR));
    float phi=atan(p.y/axisRatio,p.x);
    float rRg=max(2.0*diskR,2.0*uIsco);
    float omega=1.0/(pow(rRg,1.5)+uSpin);
    float beta=clamp(abs(rRg*omega),0.0,0.78);
    float gamma=1.0/sqrt(max(0.05,1.0-beta*beta));
    float lineOfSight=sin(uInclination)*cos(phi)*sign(omega);
    float grav=sqrt(max(0.035,1.0-2.0/rRg+(uSpin*uSpin)/(rRg*rRg)));
    float gShift=clamp(grav/(gamma*(1.0-beta*lineOfSight)),0.18,2.4);
    float zeroTorque=max(0.0,1.0-sqrt(uIsco/max(diskR,uIsco+0.001)));
    float flux=pow(max(diskR/uIsco,1.0),-3.0)*zeroTorque;
    float arcs=noise(vec2(phi*11.0-uTime*0.12,diskR*1.2));
    float spiral=0.72+0.28*sin(phi*9.0-diskR*3.0+uTime*0.5);
    float density=diskRing*pow(max(flux,0.0),0.28)*(0.58+0.55*arcs)*spiral;
    float temperature=clamp(pow(max(flux,1e-7),0.25)*gShift*2.4,0.0,1.0);
    vec3 thermal=mix(vec3(0.82,0.075,0.012),vec3(0.72,0.86,1.0),temperature);
    vec3 disk=thermal*density*pow(gShift,3.0)*2.15;

    // First higher-order image of the far side of the disk. In a full ray
    // tracer this is found by a second equatorial-plane crossing; the capture
    // boundary and its azimuthal Kerr displacement are exact here, while the
    // emissivity mapping along that boundary is a real-time approximation.
    float imageRadius=shadowBoundary+0.34+0.06*cos(phi*2.0);
    float imageWidth=mix(0.10,0.22,1.0-axisRatio);
    float higherImage=exp(-pow((b-imageRadius)/imageWidth,2.0));
    float imageStrands=0.58+0.42*noise(vec2(phi*18.0-uTime*0.09,b*5.0));
    float imageHemisphere=mix(0.68,1.18,smoothstep(-0.25,0.25,p.y));
    vec3 lensedDisk=mix(vec3(1.0,0.34,0.08),vec3(1.0,0.91,0.67),temperature)
                    *higherImage*imageStrands*imageHemisphere*pow(gShift,2.2)*1.55;
    float halo=0.014*exp(-pow((b-6.0)/5.0,2.0));
    col+=disk+lensedDisk+vec3(1.0,0.31,0.11)*halo;
    // Exact Kerr capture boundary, sampled from spherical null geodesics.
    float photon=exp(-pow((b-shadowBoundary-0.115)/0.085,2.0));
    float subring=exp(-pow((b-shadowBoundary-0.035)/0.018,2.0));
    col+=vec3(1.0,0.65,0.30)*photon*1.15+vec3(1.0,0.91,0.72)*subring*1.35;
    float outsideShadow=smoothstep(shadowBoundary-0.025,shadowBoundary+0.015,b);
    col*=outsideShadow;
    col=vec3(1.0)-exp(-col*1.25); // simple tone mapping
    float shadowOpacity=1.0-outsideShadow;
    float emissionOpacity=clamp(density*2.0+higherImage+photon+subring+halo*18.0,0.0,0.98);
    float bentSkyOpacity=0.08*clamp(alpha/2.0,0.0,1.0);
    fragColor=vec4(pow(col,vec3(1.0/2.2)),
                   max(shadowOpacity,max(emissionOpacity,bentSkyOpacity)));
}
)GLSL";
}

GLView::GLView(QWidget* parent):QOpenGLWidget(parent) {
    setMinimumSize(550,450);
    setFocusPolicy(Qt::StrongFocus);
    galaxy_.reset(100000);
    center_.reset();
    physicsLab_.reset();
    std::mt19937 rng(87);
    std::uniform_real_distribution<float> u(-1.0f,1.0f),brightness(0.4f,0.9f);
    backdrop_.reserve(850);
    for(int i=0;i<850;++i) {
        const float x=u(rng)*90000,y=u(rng)*90000,z=u(rng)*28000;
        const float v=brightness(rng);
        backdrop_.push_back({x,y,z,0.55f*v,0.72f*v,v,1.0f});
    }
    connect(&timer_,&QTimer::timeout,this,&GLView::tick);
    timer_.start(16);
    clock_.start();
}

GLView::~GLView() {
    if(context()) {
        makeCurrent();
        if(starsVBO_) glDeleteBuffers(1,&starsVBO_);
        if(starsVAO_) glDeleteVertexArrays(1,&starsVAO_);
        if(linesVBO_) glDeleteBuffers(1,&linesVBO_);
        if(linesVAO_) glDeleteVertexArrays(1,&linesVAO_);
        if(quadVBO_) glDeleteBuffers(1,&quadVBO_);
        if(quadVAO_) glDeleteVertexArrays(1,&quadVAO_);
        if(spacetimeVBO_) glDeleteBuffers(1,&spacetimeVBO_);
        if(spacetimeVAO_) glDeleteVertexArrays(1,&spacetimeVAO_);
        if(physicsGridVBO_) glDeleteBuffers(1,&physicsGridVBO_);
        if(physicsGridVAO_) glDeleteVertexArrays(1,&physicsGridVAO_);
        if(photonDiagramVBO_) glDeleteBuffers(1,&photonDiagramVBO_);
        if(photonDiagramVAO_) glDeleteVertexArrays(1,&photonDiagramVAO_);
        if(physicsColorTex_) glDeleteTextures(1,&physicsColorTex_);
        if(physicsFbo_) glDeleteFramebuffers(1,&physicsFbo_);
        if(lensTex_) glDeleteTextures(1,&lensTex_);
        if(kerrShadowTex_) glDeleteTextures(1,&kerrShadowTex_);
        doneCurrent();
    }
}

void GLView::setMode(Mode next) {
    if(mode_==next) return;
    if(mode_==Mode::Physics && next!=Mode::Physics && photonDiagramEnabled_) {
        photonDiagramEnabled_=false;
        emit photonDiagramChanged(false);
    }
    mode_=next;
    galaxyDirty_=true; // Center and galaxy share the same GPU VBO.
    resetCamera();
    clock_.restart();
    emit modeChanged(int(mode_));
    emit statsUpdated(statistics());
    update();
}

void GLView::resetCamera() {
    if(mode_==Mode::Physics) {
        yaw_=0.0f;pitch_=0.0f;distance_=6.34194e10f/1.269e10f;
        cameraTarget_=QVector3D(0,0,0);blackHoleScale_=14.f;
    } else {
        const bool compactScene=mode_==Mode::BlackHole;
        yaw_=-70.f;
        pitch_=(compactScene?15.f:37.f);
        distance_=(mode_==Mode::MilkyWay?75.f:(compactScene?28.f:21000.f));
        cameraTarget_=(compactScene?QVector3D(0,0,-6.95f):QVector3D(0,0,0));
        blackHoleScale_=(compactScene?34.f:14.f);
    }
    kerrLutDirty_=true;
    update();
}

void GLView::setPaused(bool p) {
    paused_=p;
    emit pauseChanged(p);
}
void GLView::setHaloEnabled(bool enable) { galaxy_.setDarkMatter(enable);galaxyDirty_=true;update(); }
void GLView::setStarCount(int count) {
    galaxy_.reset(std::size_t(std::clamp(count,1000,1000000)));
    galaxyDirty_=true;update();
    emit statsUpdated(statistics());
}
void GLView::setBlackHoleMass(double massSolar) {
    galaxy_.setBlackHoleMass(massSolar);
    center_.reset(massSolar,center_.blackHole().dimensionlessSpin);
    centerOrbitsDirty_=true;
    update();
    emit statsUpdated(statistics());
}
void GLView::setBlackHoleSpin(double spin) {
    center_.setBlackHoleSpin(spin);
    kerrLutDirty_=true;
    update();
    emit statsUpdated(statistics());
}
void GLView::setPhysicsGravityEnabled(bool enabled) {
    physicsLab_.setGravityEnabled(enabled);
    emit physicsGravityChanged(enabled);
    emit statsUpdated(statistics());
    update();
}
void GLView::setPhotonDiagramEnabled(bool enabled) {
    if(photonDiagramEnabled_==enabled) return;
    photonDiagramEnabled_=enabled;
    shaderTime_=0.0;
    if(enabled && paused_) setPaused(false);
    emit photonDiagramChanged(enabled);
    emit statsUpdated(statistics());
    update();
}
void GLView::resetCurrent() {
    if(mode_==Mode::MilkyWay) galaxy_.reset(galaxy_.stars().size());
    if(mode_==Mode::BlackHole) shaderTime_=0.0;
    if(mode_==Mode::Physics) {
        physicsLab_.reset();shaderTime_=0.0;
        emit physicsGravityChanged(physicsLab_.gravityEnabled());
    }
    if(mode_==Mode::GalacticCenter) {
        center_.reset(center_.blackHoleMassSolar(),center_.blackHole().dimensionlessSpin);
        centerOrbitsDirty_=true;
    }
    galaxyDirty_=true;resetCamera();
    emit statsUpdated(statistics());
}
QString GLView::statistics() const {
    if(mode_==Mode::MilkyWay)
        return QString("Звёзд: %1\nВремя: %2 млн лет\nГало тёмной материи: %3\nМетод: Leapfrog + аналитический потенциал")
            .arg(qlonglong(galaxy_.stars().size())).arg(galaxy_.timeMyr(),0,'f',2)
            .arg(galaxy_.params().darkMatter?"включено":"выключено");
    if(mode_==Mode::GalacticCenter) {
        const auto& s=center_.stars().front();
        const auto& bh=center_.blackHole();
        const double speedMs=bh.vel.norm()*astro::AU_KM*1000.0/(365.25*86400.0);
        return QString("Масса: %1 млн M☉  •  a*: %2\nВремя: %3 лет\nS2, расстояние: %4 AU\nСкорость ЧД: %5 м/с\nАктивно: %6  •  TDE: %7  •  захват: %8\nМетод: N-body + 1PN + Лензе—Тирринг")
            .arg(center_.blackHoleMassSolar()/1e6,0,'f',2)
            .arg(bh.dimensionlessSpin,0,'f',3)
            .arg(center_.timeYears(),0,'f',2).arg((s.pos-bh.pos).norm(),0,'f',1)
            .arg(speedMs,0,'f',3).arg(center_.activeObjectCount())
            .arg(center_.disruptedCount()).arg(center_.capturedCount());
    }
    if(mode_==Mode::Physics) {
        if(photonDiagramEnabled_)
            return QString("PHYSICS + PHOTON TRAJECTORIES\nОбъектов: %1 (включая ЧД)\nЛучи: %2\nГоризонт: 1 Rs  •  фотонная сфера: 1.5 Rs\nКритический impact parameter: 2.598 Rs\nБелые лучи уходят, оранжевые захватываются\nИнтегратор: Schwarzschild null geodesics / RK4")
                .arg(physicsLab_.bodies().size()+1).arg(diagramRayOffsets_.size());
        return QString("PHYSICS / REFERENCE VIEW\nОбъектов: %1 (включая ЧД)\nВремя: %2 Rs/c\nГравитация объектов: %3\nЗахвачено: %4\nRay tracer: Schwarzschild, 200×150\nСетка: 25×25, динамическая геометрия\nBackend: OpenGL 3.3 fragment pass")
            .arg(physicsLab_.bodies().size()+1).arg(physicsLab_.timeRsOverC(),0,'f',2)
            .arg(physicsLab_.gravityEnabled()?"включена":"выключена")
            .arg(physicsLab_.capturedCount());
    }
    const auto& bh=center_.blackHole();
    const double rgKm=0.5*bh.massSolar*astro::SCHWARZSCHILD_KM_PER_MSUN;
    const double horizonKm=astro::kerrHorizonRadiusRg(bh.dimensionlessSpin)*rgKm;
    const double iscoRg=astro::kerrIscoRadiusRg(bh.dimensionlessSpin);
    return QString("Масса: %1 млн M☉  •  спин a*: %2\nГоризонт r+: %3 млн км\nISCO: %4 r_g\nНаклон наблюдателя: %5°\nСетка: срез Фламма\nЛучи: нулевые геодезические + RK4\nОранжевые лучи захватываются горизонтом")
        .arg(center_.blackHoleMassSolar()/1e6,0,'f',2)
        .arg(bh.dimensionlessSpin,0,'f',3).arg(horizonKm/1e6,0,'f',2)
        .arg(iscoRg,0,'f',3).arg(blackHoleInclination()*180.0/astro::PI,0,'f',1);
}

void GLView::tick() {
    const double dt=std::min(0.05,double(clock_.restart())/1000.0);
    bool redraw=false;
    if(!paused_) {
        redraw=true;
        shaderTime_+=dt*((mode_==Mode::BlackHole || mode_==Mode::Physics)
                         ?std::sqrt(timeScale_):1.0);
        if(mode_==Mode::MilkyWay) {
            galaxy_.step(dt*timeScale_);
            galaxyDirty_=true;
        } else if(mode_==Mode::GalacticCenter) {
            center_.step(dt*timeScale_);
        } else if(mode_==Mode::Physics) {
            physicsLab_.step(dt*std::sqrt(timeScale_)*1.7);
        }
    }
    if(++frameCount_%12==0) emit statsUpdated(statistics());
    if(redraw) update();
}

void GLView::createPrograms() {
    auto compile=[](QOpenGLShaderProgram& program,const char* vs,const char* fs) {
        if(!program.addShaderFromSourceCode(QOpenGLShader::Vertex,vs)
           || !program.addShaderFromSourceCode(QOpenGLShader::Fragment,fs)
           || !program.link()) {
            qWarning().noquote()<<"Shader compilation failed:"<<program.log();
            return false;
        }
        return true;
    };
    glReady_=compile(starProgram_,starVertex,starFragment)
            &&compile(lineProgram_,lineVertex,lineFragment)
            &&compile(blackHoleProgram_,quadVertex,blackHoleFragment)
            &&compile(spacetimeProgram_,spacetimeVertex,spacetimeFragment)
            &&compile(physicsGridProgram_,physicsGridVertex,physicsGridFragment)
            &&compile(physicsRayProgram_,quadVertex,physicsRayFragment)
            &&compile(physicsTextureProgram_,quadVertex,physicsTextureFragment);
}

void GLView::initializeGL() {
    initializeOpenGLFunctions();
    createPrograms();
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glDisable(GL_DEPTH_TEST);

    glGenVertexArrays(1,&starsVAO_);glBindVertexArray(starsVAO_);
    glGenBuffers(1,&starsVBO_);glBindBuffer(GL_ARRAY_BUFFER,starsVBO_);
    starVboCapacityBytes_=sizeof(Vertex)*100000;
    glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(starVboCapacityBytes_),nullptr,GL_STREAM_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,x)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,size)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1,&linesVAO_);glBindVertexArray(linesVAO_);
    glGenBuffers(1,&linesVBO_);glBindBuffer(GL_ARRAY_BUFFER,linesVBO_);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);

    const float quad[]={-1.f,-1.f,1.f,-1.f,-1.f,1.f,1.f,-1.f,1.f,1.f,-1.f,1.f};
    glGenVertexArrays(1,&quadVAO_);glBindVertexArray(quadVAO_);
    glGenBuffers(1,&quadVBO_);glBindBuffer(GL_ARRAY_BUFFER,quadVBO_);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    buildSpacetimeGeometry();
    glGenVertexArrays(1,&spacetimeVAO_);glBindVertexArray(spacetimeVAO_);
    glGenBuffers(1,&spacetimeVBO_);glBindBuffer(GL_ARRAY_BUFFER,spacetimeVBO_);
    glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(spacetimeVertices_.size()*sizeof(float)),
                 spacetimeVertices_.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    buildPhotonDiagramGeometry();
    glGenVertexArrays(1,&photonDiagramVAO_);glBindVertexArray(photonDiagramVAO_);
    glGenBuffers(1,&photonDiagramVBO_);glBindBuffer(GL_ARRAY_BUFFER,photonDiagramVBO_);
    glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(photonDiagramVertices_.size()*sizeof(float)),
                 photonDiagramVertices_.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    constexpr int referenceGridSize=25;
    constexpr float referenceSpacing=1.0e10f/1.269e10f;
    std::vector<float> physicsGrid;
    physicsGrid.reserve(referenceGridSize*referenceGridSize*4*3);
    auto gridVertex=[&](int x,int z) {
        physicsGrid.push_back(float(x-referenceGridSize/2)*referenceSpacing);
        physicsGrid.push_back(0.0f);
        physicsGrid.push_back(float(z-referenceGridSize/2)*referenceSpacing);
    };
    for(int z=0;z<referenceGridSize;++z) for(int x=0;x<referenceGridSize;++x) {
        gridVertex(x,z);gridVertex(x+1,z);
        gridVertex(x,z);gridVertex(x,z+1);
    }
    physicsGridVertexCount_=int(physicsGrid.size()/3);
    glGenVertexArrays(1,&physicsGridVAO_);glBindVertexArray(physicsGridVAO_);
    glGenBuffers(1,&physicsGridVBO_);glBindBuffer(GL_ARRAY_BUFFER,physicsGridVBO_);
    glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(physicsGrid.size()*sizeof(float)),
                 physicsGrid.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glGenFramebuffers(1,&physicsFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER,physicsFbo_);
    glGenTextures(1,&physicsColorTex_);
    glBindTexture(GL_TEXTURE_2D,physicsColorTex_);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,200,150,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,
                           physicsColorTex_,0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        qWarning()<<"Physics ray-tracing framebuffer is incomplete";
        glReady_=false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());

    const auto lut=astro::makeDeflectionLut(2048);
    glGenTextures(1,&lensTex_);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_1D,lensTex_);
    glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,GLsizei(lut.size()),0,GL_RED,GL_FLOAT,lut.data());
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);

    const auto shadow=astro::makeKerrShadowLut(2048,center_.blackHole().dimensionlessSpin,
                                                blackHoleInclination());
    glGenTextures(1,&kerrShadowTex_);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_1D,kerrShadowTex_);
    glTexImage1D(GL_TEXTURE_1D,0,GL_R32F,GLsizei(shadow.size()),0,GL_RED,GL_FLOAT,shadow.data());
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    kerrLutDirty_=false;
}

void GLView::resizeGL(int,int) {}

QMatrix4x4 GLView::cameraMatrix() const {
    if(mode_==Mode::Physics) return physicsCameraMatrix();
    const float aspect=float(std::max(1,width()))/float(std::max(1,height()));
    QMatrix4x4 projection;
    if(mode_==Mode::MilkyWay) projection.perspective(54.f,aspect,0.01f,3000.f);
    else if(mode_==Mode::BlackHole || mode_==Mode::Physics)
        projection.perspective(52.f,aspect,0.05f,240.f);
    else projection.perspective(51.f,aspect,0.1f,1000000.f);
    QMatrix4x4 view;
    const float yaw=yaw_*float(astro::PI/180.0),pitch=pitch_*float(astro::PI/180.0);
    QVector3D camera(distance_*std::cos(pitch)*std::cos(yaw),
                     distance_*std::cos(pitch)*std::sin(yaw),
                     distance_*std::sin(pitch));
    view.lookAt(camera+cameraTarget_,cameraTarget_,QVector3D(0,0,1));
    return projection*view;
}

QMatrix4x4 GLView::physicsCameraMatrix() const {
    QMatrix4x4 projection;
    projection.perspective(60.0f,4.0f/3.0f,0.08f,200.0f);
    const float azimuth=yaw_*float(astro::PI/180.0);
    const float elevation=pitch_*float(astro::PI/180.0);
    const QVector3D camera(distance_*std::cos(elevation)*std::cos(azimuth),
                           distance_*std::sin(elevation),
                           distance_*std::cos(elevation)*std::sin(azimuth));
    QMatrix4x4 view;
    view.lookAt(camera,QVector3D(0,0,0),QVector3D(0,1,0));
    return projection*view;
}

QMatrix4x4 GLView::photonDiagramMatrix() const {
    const float aspect=float(std::max(1,width()))/float(std::max(1,height()));
    constexpr float halfWidth=10.5f;
    const float halfHeight=halfWidth/std::max(0.35f,aspect);
    QMatrix4x4 projection;
    projection.ortho(-halfWidth,halfWidth,-halfHeight,halfHeight,-1.0f,1.0f);
    return projection;
}

void GLView::uploadVertices(const std::vector<Vertex>& vertices) {
    glBindVertexArray(starsVAO_);
    glBindBuffer(GL_ARRAY_BUFFER,starsVBO_);
    const std::size_t bytes=vertices.size()*sizeof(Vertex);
    if(bytes>starVboCapacityBytes_) {
        starVboCapacityBytes_=std::max(bytes,starVboCapacityBytes_*2);
        glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(starVboCapacityBytes_),nullptr,GL_STREAM_DRAW);
    }
    if(bytes) glBufferSubData(GL_ARRAY_BUFFER,0,GLsizeiptr(bytes),vertices.data());
}

void GLView::drawGalaxy() {
    if(galaxyDirty_) {
        galaxyVertices_.clear();galaxyVertices_.reserve(galaxy_.stars().size());
        for(const auto& s:galaxy_.stars())
            galaxyVertices_.push_back({float(s.pos.x),float(s.pos.y),float(s.pos.z),s.r,s.g,s.b,s.size});
        uploadVertices(galaxyVertices_);
        galaxyDirty_=false;
    } else glBindVertexArray(starsVAO_);
    starProgram_.bind();
    starProgram_.setUniformValue("uMVP",cameraMatrix());
    starProgram_.setUniformValue("uPointScale",2.2f);
    glDrawArrays(GL_POINTS,0,GLsizei(galaxyVertices_.size()));
    starProgram_.release();
    glBindVertexArray(0);
}

void GLView::rebuildCenterOrbitBuffer() {
    centerOrbitVertices_.clear();centerOrbitOffsets_.clear();centerOrbitCounts_.clear();
    centerOrbitVertices_.reserve(center_.stars().size()*280*3);
    for(const auto& s:center_.stars()) {
        const auto track=center_.referenceOrbit(s,280);
        centerOrbitOffsets_.push_back(int(centerOrbitVertices_.size()/3));
        centerOrbitCounts_.push_back(int(track.size()));
        for(const auto& p:track) {
            centerOrbitVertices_.push_back(float(p.x));
            centerOrbitVertices_.push_back(float(p.y));
            centerOrbitVertices_.push_back(float(p.z));
        }
    }
    glBindVertexArray(linesVAO_);
    glBindBuffer(GL_ARRAY_BUFFER,linesVBO_);
    glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(centerOrbitVertices_.size()*sizeof(float)),
                 centerOrbitVertices_.data(),GL_STATIC_DRAW);
    glBindVertexArray(0);
    centerOrbitsDirty_=false;
}

double GLView::blackHoleInclination() const {
    const double elevation=double(pitch_)*astro::PI/180.0;
    return std::acos(std::clamp(std::abs(std::sin(elevation)),0.0,1.0));
}

void GLView::updateKerrShadowTexture() {
    if(!kerrLutDirty_ || !kerrShadowTex_) return;
    const auto shadow=astro::makeKerrShadowLut(2048,center_.blackHole().dimensionlessSpin,
                                                blackHoleInclination());
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_1D,kerrShadowTex_);
    glTexSubImage1D(GL_TEXTURE_1D,0,0,GLsizei(shadow.size()),GL_RED,GL_FLOAT,shadow.data());
    kerrLutDirty_=false;
}

void GLView::buildSpacetimeGeometry() {
    spacetimeVertices_.clear();
    photonPathOffsets_.clear();photonPathCounts_.clear();
    constexpr int layers=52,sectors=112;
    constexpr double rMin=1.015,rMax=14.0;
    const double farHeight=astro::flammEmbeddingHeightRs(rMax);
    auto radiusAt=[=](double u) {
        return rMin+(rMax-rMin)*u*u;
    };
    auto vertex=[&](double radius,double phi) {
        const double z=astro::flammEmbeddingHeightRs(radius)-farHeight;
        spacetimeVertices_.push_back(float(radius*std::cos(phi)));
        spacetimeVertices_.push_back(float(radius*std::sin(phi)));
        spacetimeVertices_.push_back(float(z));
    };

    // A dark surface provides depth occlusion; the visible structure is drawn
    // by the physically parameterized radial and azimuthal grid below.
    for(int ring=0;ring<layers-1;++ring) {
        const double r0=radiusAt(double(ring)/(layers-1));
        const double r1=radiusAt(double(ring+1)/(layers-1));
        for(int j=0;j<sectors;++j) {
            const double p0=2.0*astro::PI*j/sectors;
            const double p1=2.0*astro::PI*(j+1)/sectors;
            vertex(r0,p0);vertex(r1,p0);vertex(r1,p1);
            vertex(r0,p0);vertex(r1,p1);vertex(r0,p1);
        }
    }
    spacetimeSurfaceCount_=int(spacetimeVertices_.size()/3);
    spacetimeGridFirst_=spacetimeSurfaceCount_;

    constexpr int spokes=32;
    for(int spoke=0;spoke<spokes;++spoke) {
        const double phi=2.0*astro::PI*spoke/spokes;
        for(int ring=0;ring<layers-1;++ring) {
            vertex(radiusAt(double(ring)/(layers-1)),phi);
            vertex(radiusAt(double(ring+1)/(layers-1)),phi);
        }
    }
    constexpr int visibleRings=27;
    for(int ring=0;ring<visibleRings;++ring) {
        const double u=double(ring)/(visibleRings-1);
        const double radius=radiusAt(u);
        for(int j=0;j<sectors;++j) {
            vertex(radius,2.0*astro::PI*j/sectors);
            vertex(radius,2.0*astro::PI*(j+1)/sectors);
        }
    }
    spacetimeGridCount_=int(spacetimeVertices_.size()/3)-spacetimeGridFirst_;
    spacetimeHorizonFirst_=int(spacetimeVertices_.size()/3);
    for(int j=0;j<sectors;++j) {
        vertex(rMin,2.0*astro::PI*j/sectors);
        vertex(rMin,2.0*astro::PI*(j+1)/sectors);
    }
    spacetimeHorizonCount_=int(spacetimeVertices_.size()/3)-spacetimeHorizonFirst_;

    // The same Schwarzschild geometry is now visible in two complementary ways:
    // as an embedding grid and as exact equatorial null-geodesic trajectories.
    for(const double impact:{2.20,2.45,2.62,2.80,3.20,4.00,5.50}) {
        const auto path=astro::schwarzschildPhotonPath(impact,rMax,7000);
        photonPathOffsets_.push_back(int(spacetimeVertices_.size()/3));
        photonPathCounts_.push_back(int(path.size()));
        for(const auto& p:path) {
            const double radius=std::max(rMin,std::hypot(p.x,p.y));
            spacetimeVertices_.push_back(float(p.x));
            spacetimeVertices_.push_back(float(p.y));
            spacetimeVertices_.push_back(float(astro::flammEmbeddingHeightRs(radius)
                                                    -farHeight+0.14));
        }
    }
}

void GLView::buildPhotonDiagramGeometry() {
    photonDiagramVertices_.clear();
    diagramRayOffsets_.clear();diagramRayCounts_.clear();
    auto append=[&](double x,double y) {
        photonDiagramVertices_.push_back(float(x));
        photonDiagramVertices_.push_back(float(y));
        photonDiagramVertices_.push_back(0.0f);
    };

    // Parallel rays enter from the right after mirroring the integrator's
    // conventional left-to-right launch. The two innermost families cross the
    // capture threshold b_c=3*sqrt(3)/2 Rs; near-critical rays wind around the
    // photon sphere before escaping or entering the horizon.
    for(const double impact:{6.0,5.0,4.0,3.4,3.0,2.72,2.61,2.56,2.35}) {
        const auto path=astro::schwarzschildPhotonPath(impact,14.0,12000);
        diagramRayOffsets_.push_back(int(photonDiagramVertices_.size()/3));
        diagramRayCounts_.push_back(int(path.size()));
        for(const auto& point:path) append(-point.x,point.y);
    }

    auto addCircle=[&](double radius,bool dashed,int& first,int& count) {
        first=int(photonDiagramVertices_.size()/3);
        constexpr int segments=192;
        for(int segment=0;segment<segments;++segment) {
            if(dashed && segment%3==2) continue;
            const double a0=2.0*astro::PI*segment/segments;
            const double a1=2.0*astro::PI*(segment+1)/segments;
            append(radius*std::cos(a0),radius*std::sin(a0));
            append(radius*std::cos(a1),radius*std::sin(a1));
        }
        count=int(photonDiagramVertices_.size()/3)-first;
    };
    addCircle(1.0,false,diagramHorizonFirst_,diagramHorizonCount_);
    addCircle(1.5,true,diagramPhotonSphereFirst_,diagramPhotonSphereCount_);
    addCircle(1.5*std::sqrt(3.0),true,diagramCriticalFirst_,diagramCriticalCount_);
}

void GLView::drawCenter() {
    const QMatrix4x4 mvp=cameraMatrix();
    if(centerOrbitsDirty_) rebuildCenterOrbitBuffer();
    glBindVertexArray(linesVAO_);
    lineProgram_.bind();
    lineProgram_.setUniformValue("uMVP",mvp);
    glLineWidth(1.0f);
    for(std::size_t k=0;k<centerOrbitOffsets_.size();++k) {
        lineProgram_.setUniformValue("uColor",k==0?QVector3D(0.25f,0.6f,1.0f)
                                                  :QVector3D(0.2f,0.34f,0.47f));
        glDrawArrays(GL_LINE_STRIP,centerOrbitOffsets_[k],centerOrbitCounts_[k]);
    }
    lineProgram_.release();
    glBindVertexArray(0);
    centerVertices_=backdrop_;
    centerVertices_.reserve(backdrop_.size()+center_.stars().size()+1);
    for(const auto& s:center_.stars()) if(s.state==astro::ObjectState::Active)
        centerVertices_.push_back({float(s.pos.x),float(s.pos.y),float(s.pos.z),s.r,s.g,s.b,
                                   s.name.find("S2")!=std::string::npos?6.0f:3.4f});
    const auto& bh=center_.blackHole();
    // Visual marker: central mass, intentionally not a sphere to scale.
    centerVertices_.push_back({float(bh.pos.x),float(bh.pos.y),float(bh.pos.z),
                               1.0f,0.37f,0.10f,11.f});
    uploadVertices(centerVertices_);
    starProgram_.bind();
    starProgram_.setUniformValue("uMVP",mvp);
    starProgram_.setUniformValue("uPointScale",2.1f);
    glDrawArrays(GL_POINTS,0,GLsizei(centerVertices_.size()));
    starProgram_.release();
    glBindVertexArray(0);
}

void GLView::drawBlackHole() {
    // One coherent scene: the curved spatial grid and null geodesics are drawn
    // in the same camera frame, then the Kerr shadow/disk is alpha-composited
    // over them so the horizon actually occludes captured trajectories.
    drawSpacetime(false);
    drawBlackHoleOverlay();
}

void GLView::drawBlackHoleOverlay() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    updateKerrShadowTexture();
    glBindVertexArray(quadVAO_);
    glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_1D,lensTex_);
    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_1D,kerrShadowTex_);
    blackHoleProgram_.bind();
    blackHoleProgram_.setUniformValue("uLens",0);
    blackHoleProgram_.setUniformValue("uShadow",1);
    blackHoleProgram_.setUniformValue("uAspect",float(std::max(1,width()))/float(std::max(1,height())));
    blackHoleProgram_.setUniformValue("uScale",blackHoleScale_);
    blackHoleProgram_.setUniformValue("uTime",float(shaderTime_));
    blackHoleProgram_.setUniformValue("uYaw",yaw_);
    blackHoleProgram_.setUniformValue("uPitch",pitch_);
    blackHoleProgram_.setUniformValue("uSpin",float(center_.blackHole().dimensionlessSpin));
    blackHoleProgram_.setUniformValue("uInclination",float(blackHoleInclination()));
    blackHoleProgram_.setUniformValue("uIsco",float(0.5*astro::kerrIscoRadiusRg(
                                                    center_.blackHole().dimensionlessSpin)));
    glDrawArrays(GL_TRIANGLES,0,6);
    blackHoleProgram_.release();
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
}

void GLView::drawSpacetime(bool physicsLab) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindVertexArray(spacetimeVAO_);
    spacetimeProgram_.bind();
    spacetimeProgram_.setUniformValue("uMVP",cameraMatrix());
    spacetimeProgram_.setUniformValue("uSpin",float(center_.blackHole().dimensionlessSpin));
    spacetimeProgram_.setUniformValue("uTime",float(shaderTime_));
    QVector4D bodyUniforms[3]={QVector4D(),QVector4D(),QVector4D()};
    int bodyCount=0;
    if(physicsLab) {
        for(const auto& body:physicsLab_.bodies()) {
            if(!body.active || bodyCount>=3) continue;
            bodyUniforms[bodyCount++]=QVector4D(float(body.pos.x),float(body.pos.y),
                                                0.0f,float(body.visualWell));
        }
    }
    spacetimeProgram_.setUniformValue("uBodyCount",bodyCount);
    spacetimeProgram_.setUniformValue("uBody0",bodyUniforms[0]);
    spacetimeProgram_.setUniformValue("uBody1",bodyUniforms[1]);
    spacetimeProgram_.setUniformValue("uBody2",bodyUniforms[2]);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    spacetimeProgram_.setUniformValue("uColor",QVector4D(0.004f,0.018f,0.055f,0.92f));
    glDrawArrays(GL_TRIANGLES,0,spacetimeSurfaceCount_);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    spacetimeProgram_.setUniformValue("uColor",QVector4D(0.08f,0.64f,1.0f,0.66f));
    glDrawArrays(GL_LINES,spacetimeGridFirst_,spacetimeGridCount_);
    spacetimeProgram_.setUniformValue("uColor",QVector4D(1.0f,0.46f,0.12f,0.95f));
    glDrawArrays(GL_LINES,spacetimeHorizonFirst_,spacetimeHorizonCount_);

    spacetimeProgram_.setUniformValue("uSpin",0.0f);
    glLineWidth(1.5f);
    for(std::size_t k=0;k<photonPathOffsets_.size();++k) {
        const bool captured=k<2;
        spacetimeProgram_.setUniformValue("uColor",captured
            ?QVector4D(1.0f,0.34f,0.08f,0.96f)
            :QVector4D(0.78f,0.92f,1.0f,0.88f));
        glDrawArrays(GL_LINE_STRIP,photonPathOffsets_[k],photonPathCounts_[k]);
    }
    glDisable(GL_PROGRAM_POINT_SIZE);
    glPointSize(5.0f);
    for(std::size_t k=0;k<photonPathOffsets_.size();++k) {
        const int count=photonPathCounts_[k];
        if(count<=0) continue;
        const double phase=std::fmod(shaderTime_*0.17+double(k)*0.137,1.0);
        const int cursor=std::clamp(int(phase*count),0,count-1);
        spacetimeProgram_.setUniformValue("uColor",k<2
            ?QVector4D(1.0f,0.72f,0.22f,1.0f)
            :QVector4D(0.90f,0.98f,1.0f,1.0f));
        glDrawArrays(GL_POINTS,photonPathOffsets_[k]+cursor,1);
    }
    glPointSize(1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(1.0f);
    spacetimeProgram_.release();
    glBindVertexArray(0);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
}

void GLView::renderPhysicsRayTexture() {
    if(physicsLab_.bodies().size()<2) return;
    const auto& first=physicsLab_.bodies()[0];
    const auto& second=physicsLab_.bodies()[1];
    const QVector4D object0(float(first.pos.x),0.0f,float(first.pos.y),
                            float(first.radiusRs));
    const QVector4D object1(float(second.pos.x),0.0f,float(second.pos.y),
                            float(second.radiusRs));
    const float azimuth=yaw_*float(astro::PI/180.0);
    const float elevation=pitch_*float(astro::PI/180.0);
    const QVector3D camera(distance_*std::cos(elevation)*std::cos(azimuth),
                           distance_*std::sin(elevation),
                           distance_*std::cos(elevation)*std::sin(azimuth));
    const QVector3D forward=(-camera).normalized();
    const QVector3D worldUp(0,1,0);
    const QVector3D right=QVector3D::crossProduct(forward,worldUp).normalized();
    const QVector3D up=QVector3D::crossProduct(right,forward).normalized();

    glBindFramebuffer(GL_FRAMEBUFFER,physicsFbo_);
    glViewport(0,0,200,150);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(quadVAO_);
    physicsRayProgram_.bind();
    physicsRayProgram_.setUniformValue("uCameraPos",camera);
    physicsRayProgram_.setUniformValue("uCameraRight",right);
    physicsRayProgram_.setUniformValue("uCameraUp",up);
    physicsRayProgram_.setUniformValue("uCameraForward",forward);
    physicsRayProgram_.setUniformValue("uObject0",object0);
    physicsRayProgram_.setUniformValue("uObject1",object1);
    glDrawArrays(GL_TRIANGLES,0,6);
    physicsRayProgram_.release();
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER,defaultFramebufferObject());
    const qreal pixelRatio=devicePixelRatioF();
    glViewport(0,0,std::max(1,int(std::lround(width()*pixelRatio))),
                     std::max(1,int(std::lround(height()*pixelRatio))));
}

void GLView::drawPhysicsGrid() {
    if(physicsLab_.bodies().size()<2) return;
    const auto& first=physicsLab_.bodies()[0];
    const auto& second=physicsLab_.bodies()[1];
    const QVector4D object0(float(first.pos.x),float(first.pos.y),float(first.radiusRs),
                            float(first.visualWell));
    const QVector4D object1(float(second.pos.x),float(second.pos.y),float(second.radiusRs),
                            float(second.visualWell));
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(physicsGridVAO_);
    physicsGridProgram_.bind();
    physicsGridProgram_.setUniformValue("uMVP",physicsCameraMatrix());
    physicsGridProgram_.setUniformValue("uObject0",object0);
    physicsGridProgram_.setUniformValue("uObject1",object1);
    glDrawArrays(GL_LINES,0,physicsGridVertexCount_);
    physicsGridProgram_.release();
    glBindVertexArray(0);
}

void GLView::drawPhotonDiagram() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glBindVertexArray(photonDiagramVAO_);
    lineProgram_.bind();
    lineProgram_.setUniformValue("uMVP",photonDiagramMatrix());

    glLineWidth(1.0f);
    lineProgram_.setUniformValue("uColor",QVector3D(0.32f,0.50f,0.72f));
    glDrawArrays(GL_LINES,diagramCriticalFirst_,diagramCriticalCount_);
    lineProgram_.setUniformValue("uColor",QVector3D(1.0f,0.72f,0.18f));
    glDrawArrays(GL_LINES,diagramPhotonSphereFirst_,diagramPhotonSphereCount_);
    lineProgram_.setUniformValue("uColor",QVector3D(1.0f,0.20f,0.05f));
    glDrawArrays(GL_LINES,diagramHorizonFirst_,diagramHorizonCount_);

    glLineWidth(1.5f);
    for(std::size_t ray=0;ray<diagramRayOffsets_.size();++ray) {
        const bool captured=ray+2>=diagramRayOffsets_.size();
        lineProgram_.setUniformValue("uColor",captured
            ?QVector3D(1.0f,0.30f,0.06f)
            :QVector3D(0.68f,0.88f,1.0f));
        glDrawArrays(GL_LINE_STRIP,diagramRayOffsets_[ray],diagramRayCounts_[ray]);
    }

    glDisable(GL_PROGRAM_POINT_SIZE);
    glPointSize(7.0f);
    for(std::size_t ray=0;ray<diagramRayOffsets_.size();++ray) {
        const int count=diagramRayCounts_[ray];
        if(count<1) continue;
        const double phase=std::fmod(shaderTime_*0.16+double(ray)*0.087,1.0);
        const int cursor=std::clamp(int(phase*count),0,count-1);
        const bool captured=ray+2>=diagramRayOffsets_.size();
        lineProgram_.setUniformValue("uColor",captured
            ?QVector3D(1.0f,0.72f,0.12f)
            :QVector3D(0.92f,0.98f,1.0f));
        glDrawArrays(GL_POINTS,diagramRayOffsets_[ray]+cursor,1);
    }
    glPointSize(1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glLineWidth(1.0f);
    lineProgram_.release();
    glBindVertexArray(0);
}

void GLView::drawPhysics() {
    renderPhysicsRayTexture();
    drawPhysicsGrid();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(quadVAO_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,physicsColorTex_);
    physicsTextureProgram_.bind();
    physicsTextureProgram_.setUniformValue("uImage",0);
    glDrawArrays(GL_TRIANGLES,0,6);
    physicsTextureProgram_.release();
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    if(photonDiagramEnabled_) drawPhotonDiagram();
}

void GLView::paintGL() {
    if(mode_==Mode::Physics) glClearColor(0.0f,0.0f,0.0f,1.0f);
    else glClearColor(0.002f,0.004f,0.012f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(!glReady_) return;
    if(mode_==Mode::BlackHole) drawBlackHole();
    else if(mode_==Mode::Physics) drawPhysics();
    else if(mode_==Mode::MilkyWay) drawGalaxy();
    else drawCenter();
}

void GLView::mousePressEvent(QMouseEvent* event) {
    lastMouse_=event->position().toPoint();
    if(mode_==Mode::Physics && event->button()==Qt::RightButton)
        setPhysicsGravityEnabled(true);
    setFocus();
}
void GLView::mouseMoveEvent(QMouseEvent* event) {
    const QPoint now=event->position().toPoint();
    const QPoint change=now-lastMouse_;lastMouse_=now;
    if((event->buttons()&Qt::LeftButton)
       || (mode_==Mode::Physics && (event->buttons()&Qt::MiddleButton))) {
        const float orbitSpeed=(mode_==Mode::Physics?0.573f:0.37f);
        yaw_+=change.x()*orbitSpeed;
        pitch_=std::clamp(pitch_-change.y()*(mode_==Mode::Physics?0.573f:0.30f),
                          mode_==Mode::Physics?-89.4f:-86.f,
                          mode_==Mode::Physics?89.4f:86.f);
        if(mode_==Mode::BlackHole) kerrLutDirty_=true;
        update();
    } else if((event->buttons()&Qt::RightButton)
              && mode_!=Mode::BlackHole && mode_!=Mode::Physics) {
        const float yaw=yaw_*float(astro::PI/180.0);
        const QVector3D right(-std::sin(yaw),std::cos(yaw),0);
        const QVector3D up(0,0,1);
        const float step=distance_*0.0014f;
        cameraTarget_+=right*(-float(change.x())*step)+up*(float(change.y())*step);
        update();
    }
}
void GLView::mouseReleaseEvent(QMouseEvent* event) {
    if(mode_==Mode::Physics && event->button()==Qt::RightButton)
        setPhysicsGravityEnabled(false);
    QOpenGLWidget::mouseReleaseEvent(event);
}
void GLView::wheelEvent(QWheelEvent* event) {
    const double steps=double(event->angleDelta().y())/120.0;
    if(mode_==Mode::Physics) {
        distance_=std::clamp(distance_-float(steps*25.0e9/1.269e10),
                             float(1.0e10/1.269e10),float(1.0e12/1.269e10));
    } else if(mode_==Mode::BlackHole) {
        const float factor=float(std::pow(0.86,steps));
        blackHoleScale_=std::clamp(blackHoleScale_*factor,12.0f,80.0f);
        distance_=std::clamp(distance_*factor,10.0f,70.0f);
    } else {
        distance_=std::clamp(float(distance_*std::pow(0.83,steps)),
                             mode_==Mode::MilkyWay?0.06f:150.0f,
                             mode_==Mode::MilkyWay?900.0f:120000.0f);
        if(mode_==Mode::MilkyWay && distance_<0.14f && cameraTarget_.length()<2.0f) setMode(Mode::GalacticCenter);
        else if(mode_==Mode::GalacticCenter && distance_<300.f) setMode(Mode::BlackHole);
    }
    update();
    event->accept();
}
void GLView::mouseDoubleClickEvent(QMouseEvent*) {
    if(mode_==Mode::MilkyWay) setMode(Mode::GalacticCenter);
    else if(mode_==Mode::GalacticCenter) setMode(Mode::BlackHole);
    else if(mode_==Mode::Physics) setMode(Mode::BlackHole);
    else setMode(Mode::GalacticCenter);
}
void GLView::keyPressEvent(QKeyEvent* event) {
    if(event->key()==Qt::Key_1) setMode(Mode::MilkyWay);
    else if(event->key()==Qt::Key_2) setMode(Mode::BlackHole);
    else if(event->key()==Qt::Key_3) setMode(Mode::GalacticCenter);
    else if(event->key()==Qt::Key_4) setMode(Mode::Physics);
    else if(event->key()==Qt::Key_Space) setPaused(!paused_);
    else if(event->key()==Qt::Key_R) resetCurrent();
    else if(event->key()==Qt::Key_H && mode_==Mode::MilkyWay)
        setHaloEnabled(!haloEnabled());
    else if(event->key()==Qt::Key_G && mode_==Mode::Physics)
        setPhysicsGravityEnabled(!physicsGravityEnabled());
    else if(event->key()==Qt::Key_Escape && mode_==Mode::BlackHole) setMode(Mode::GalacticCenter);
    else if(event->key()==Qt::Key_Escape && mode_==Mode::Physics && photonDiagramEnabled_)
        setPhotonDiagramEnabled(false);
    else if(event->key()==Qt::Key_Escape && mode_==Mode::Physics) setMode(Mode::BlackHole);
    else if(event->key()==Qt::Key_Home) resetCamera();
    else if(mode_!=Mode::BlackHole && mode_!=Mode::Physics) {
        // Fly / pan around the scene with W,A,S,D,Q,E.
        const float a=yaw_*float(astro::PI/180.0);
        const QVector3D right(-std::sin(a),std::cos(a),0);
        const QVector3D forward(-std::cos(a),-std::sin(a),0);
        const float step=distance_*(mode_==Mode::MilkyWay?0.075f:0.055f);
        const int key=event->key();
        if(key==Qt::Key_W) cameraTarget_+=forward*step;
        else if(key==Qt::Key_S) cameraTarget_-=forward*step;
        else if(key==Qt::Key_A) cameraTarget_-=right*step;
        else if(key==Qt::Key_D) cameraTarget_+=right*step;
        else if(key==Qt::Key_Q) cameraTarget_.setZ(cameraTarget_.z()-step);
        else if(key==Qt::Key_E) cameraTarget_.setZ(cameraTarget_.z()+step);
        else { QOpenGLWidget::keyPressEvent(event); return; }
        update();
    } else QOpenGLWidget::keyPressEvent(event);
}
