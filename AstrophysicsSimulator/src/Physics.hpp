#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace astro {
struct Vec3 {
    double x = 0.0, y = 0.0, z = 0.0;
    Vec3 operator+(Vec3 v) const { return {x+v.x,y+v.y,z+v.z}; }
    Vec3 operator-(Vec3 v) const { return {x-v.x,y-v.y,z-v.z}; }
    Vec3 operator-() const { return {-x,-y,-z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
    Vec3 operator/(double s) const { return {x/s,y/s,z/s}; }
    Vec3& operator+=(Vec3 v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
    Vec3& operator-=(Vec3 v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
    Vec3& operator*=(double s) { x*=s; y*=s; z*=s; return *this; }
    double dot(Vec3 v) const { return x*v.x+y*v.y+z*v.z; }
    double norm2() const { return dot(*this); }
    double norm() const { return std::sqrt(norm2()); }
};
inline Vec3 operator*(double s, Vec3 v) { return v*s; }
inline Vec3 cross(Vec3 a,Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
inline constexpr double PI = 3.14159265358979323846;
inline constexpr double G_KPC_MYR = 4.498502151575286e-12; // kpc^3 / (Msun Myr^2)
inline constexpr double G_AU_YR = 4.0*PI*PI; // AU^3 / (Msun yr^2), conventional astronomical approximation
inline constexpr double C_AU_YR = 63241.077; // AU / Julian year
inline constexpr double KM_PER_S_TO_KPC_PER_MYR = 0.001022712165;
inline constexpr double SCHWARZSCHILD_KM_PER_MSUN = 2.95325008;
inline constexpr double SOLAR_RADIUS_AU = 0.00465046726096;
inline constexpr double AU_KM = 149597870.7;

struct Body {
    Vec3 pos, vel;
    float r=1.0f,g=1.0f,b=1.0f,size=1.0f;
};

struct GalaxyParams {
    bool darkMatter = true;
    double diskMass = 6.0e10;
    double bulgeMass = 1.0e10;
    double centralBlackHoleMass = 4.15e6;
    double darkHaloSpeedKms = 180.0;
};

// Physically motivated but time-independent axisymmetric background potential.
Vec3 galaxyAcceleration(Vec3 pos, const GalaxyParams& params);
double galaxyPotential(Vec3 pos, const GalaxyParams& params);

class GalaxySimulation {
public:
    void reset(std::size_t count, std::uint32_t seed=2026);
    void step(double dtMyr);
    void setDarkMatter(bool on);
    void setBlackHoleMass(double massSolar);
    const std::vector<Body>& stars() const { return stars_; }
    const GalaxyParams& params() const { return params_; }
    double timeMyr() const { return timeMyr_; }
private:
    GalaxyParams params_;
    std::vector<Body> stars_;
    std::vector<Vec3> accelerations_,nextAccelerations_;
    bool accelerationCacheValid_=false;
    double timeMyr_ = 0.0;
};

enum class ObjectState : std::uint8_t { Active, TidallyDisrupted, Captured };

struct OrbitingStar {
    std::string name;
    Vec3 pos, vel; // barycentric AU and AU/year
    float r=1,g=1,b=1;
    double semimajorAU=1000, eccentricity=0.5;
    double inclination=0.0, longitude=0.0, argumentPeriapsis=0.0;
    double massSolar=1.0,radiusSolar=1.0;
    ObjectState state=ObjectState::Active;
};

struct BlackHoleState {
    Vec3 pos,vel; // barycentric AU and AU/year
    Vec3 spinAxis={0,0,1};
    double massSolar=4.15e6;
    double dimensionlessSpin=0.65; // signed Kerr a*=cJ/(GM^2), |a*|<1
    double accretedMassSolar=0.0;
};

double gravitationalRadiusAU(double massSolar);
double kerrHorizonRadiusRg(double dimensionlessSpin);
double kerrIscoRadiusRg(double dimensionlessSpin);
double flammEmbeddingHeightRs(double radiusRs);
double kerrEquatorialFrameDraggingOmega(double radiusRg,double dimensionlessSpin);
double tidalDisruptionRadiusAU(double blackHoleMassSolar,double objectMassSolar,
                               double objectRadiusSolar);
std::vector<float> makeKerrShadowLut(std::size_t resolution,double dimensionlessSpin,
                                     double observerInclinationRadians);
// Equatorial Schwarzschild null geodesic. Coordinates are in Schwarzschild radii;
// impact <= 3*sqrt(3)/2 is captured. The returned path lies in z=0.
std::vector<Vec3> schwarzschildPhotonPath(double impactRs,double extentRs=14.0,
                                          std::size_t maxSamples=4096);

class GalacticCenterSimulation {
public:
    void reset(double massSolar=4.15e6,double dimensionlessSpin=0.65);
    void step(double dtYears);
    void setBlackHoleSpin(double dimensionlessSpin);
    const std::vector<OrbitingStar>& stars() const { return stars_; }
    const BlackHoleState& blackHole() const { return blackHole_; }
    double timeYears() const { return years_; }
    double blackHoleMassSolar() const { return blackHole_.massSolar; }
    std::size_t activeObjectCount() const;
    std::uint64_t capturedCount() const { return capturedCount_; }
    std::uint64_t disruptedCount() const { return disruptedCount_; }
    Vec3 totalMomentum() const;
    // Newtonian reference curves; N-body, 1PN and spin-orbit terms are integrated in step().
    std::vector<Vec3> referenceOrbit(const OrbitingStar& s, int samples=256) const;
private:
    void calculateAccelerations(const std::vector<Vec3>& positions,
                                const std::vector<Vec3>& velocities,
                                std::vector<Vec3>& out) const;
    void handleInteractions();
    void accrete(OrbitingStar& object,double fraction,ObjectState finalState);
    double adaptiveStep(double requested) const;
    BlackHoleState blackHole_;
    Vec3 referenceOrigin_;
    double years_=0;
    std::uint64_t capturedCount_=0,disruptedCount_=0;
    std::vector<OrbitingStar> stars_;
};

struct PhysicsLabBody {
    std::string name;
    Vec3 pos,vel; // normalized units: Schwarzschild radii and Rs/c
    double mass=0.0,radiusRs=0.18,visualWell=0.0;
    float r=1.0f,g=1.0f,b=1.0f;
    bool active=true;
};

// Cross-platform educational sandbox inspired by the linked ray-tracing demo.
// It is an original normalized-unit implementation and does not depend on
// OpenGL 4.3 compute shaders.
class PhysicsLabSimulation {
public:
    void reset();
    void step(double dtRsOverC);
    void setGravityEnabled(bool enabled) { gravityEnabled_=enabled; }
    bool gravityEnabled() const { return gravityEnabled_; }
    double timeRsOverC() const { return time_; }
    std::uint64_t capturedCount() const { return capturedCount_; }
    const std::vector<PhysicsLabBody>& bodies() const { return bodies_; }
private:
    void respawn(PhysicsLabBody& body,std::size_t index);
    std::vector<PhysicsLabBody> bodies_;
    bool gravityEnabled_=true;
    double time_=0.0;
    std::uint64_t capturedCount_=0;
};

// Bending angle for Schwarzschild photon on unbound trajectory from infinity to infinity.
// b is in Schwarzschild radii. b <= 3*sqrt(3)/2 is captured.
double schwarzschildDeflection(double b);
std::vector<float> makeDeflectionLut(std::size_t resolution, double bMax=60.0);
}
