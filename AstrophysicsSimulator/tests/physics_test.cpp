#include "Physics.hpp"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace astro;
    GalaxyParams p;
    const Vec3 r={8,0,0}, a=galaxyAcceleration(r,p);
    assert(a.x<0 && std::isfinite(a.x));
    assert(std::abs(a.y)<1e-15);

    // Force must be the negative numerical gradient of the potential.
    const double h=1e-4;
    const double numericalAx=-(galaxyPotential({r.x+h,0,0},p)-galaxyPotential({r.x-h,0,0},p))/(2*h);
    assert(std::abs((a.x-numericalAx)/a.x)<2e-7);

    GalaxySimulation galaxy;
    galaxy.reset(1200);
    assert(galaxy.stars().size()==1200);
    const auto first=galaxy.stars()[0].pos;
    const auto& star=galaxy.stars()[12];
    const double initialEnergy=0.5*star.vel.norm2()+galaxyPotential(star.pos,p);
    galaxy.step(0.1);
    assert((galaxy.stars()[0].pos-first).norm()>0);
    galaxy.step(20.0);
    const auto& moved=galaxy.stars()[12];
    const double finalEnergy=0.5*moved.vel.norm2()+galaxyPotential(moved.pos,p);
    assert(std::abs(finalEnergy-initialEnergy)/std::max(0.001,std::abs(initialEnergy))<0.005);

    GalacticCenterSimulation center;
    center.reset();
    assert(center.stars().size()>=10);
    assert(center.activeObjectCount()==center.stars().size());
    assert(std::abs(kerrHorizonRadiusRg(0.0)-2.0)<1e-14);
    assert(std::abs(kerrHorizonRadiusRg(1.0)-1.0)<1e-14);
    assert(std::abs(kerrIscoRadiusRg(0.0)-6.0)<1e-12);
    assert(kerrIscoRadiusRg(0.9)<kerrIscoRadiusRg(0.0));
    assert(kerrIscoRadiusRg(-0.9)>kerrIscoRadiusRg(0.0));
    assert(std::abs(flammEmbeddingHeightRs(1.0))<1e-14);
    assert(std::abs(flammEmbeddingHeightRs(2.0)-2.0)<1e-14);
    assert(std::isnan(flammEmbeddingHeightRs(0.99)));
    assert(kerrEquatorialFrameDraggingOmega(4.0,0.8)>0.0);
    assert(kerrEquatorialFrameDraggingOmega(4.0,-0.8)<0.0);
    const auto capturedPhoton=schwarzschildPhotonPath(2.3);
    const auto escapedPhoton=schwarzschildPhotonPath(5.0);
    const auto nearCriticalEscape=schwarzschildPhotonPath(2.61,14.0,12000);
    const auto nearCriticalCapture=schwarzschildPhotonPath(2.56,14.0,12000);
    assert(capturedPhoton.size()>100 && escapedPhoton.size()>100);
    const auto minRadius=[](const std::vector<Vec3>& path) {
        double result=1e9;
        for(const auto& p:path) result=std::min(result,p.norm());
        return result;
    };
    assert(minRadius(capturedPhoton)<1.05);
    assert(minRadius(escapedPhoton)>1.5);
    assert(minRadius(nearCriticalEscape)>1.0);
    assert(minRadius(nearCriticalCapture)<1.05);
    const double solarTidal=tidalDisruptionRadiusAU(4.15e6,1.0,1.0);
    assert(solarTidal>0.7 && solarTidal<0.8);
    auto shadow=makeKerrShadowLut(512,0.9,PI/2.0);
    const auto shadowRange=std::minmax_element(shadow.begin(),shadow.end());
    assert(shadow.size()==512 && *shadowRange.first>1.0f && *shadowRange.second<3.7f);
    assert(*shadowRange.second-*shadowRange.first>0.05f);
    auto ref=center.referenceOrbit(center.stars().front());
    assert(ref.size()==256);
    const auto initialBhPosition=center.blackHole().pos;
    const auto initialMomentum=center.totalMomentum();
    center.step(0.02);
    assert(center.stars().front().pos.norm()>0);
    assert((center.blackHole().pos-initialBhPosition).norm()>1e-12);
    assert((center.totalMomentum()-initialMomentum).norm()<1e-5);
    const double massBeforeTde=center.blackHoleMassSolar();
    center.step(0.88);
    assert(center.disruptedCount()>=1);
    assert(center.blackHoleMassSolar()>massBeforeTde);

    PhysicsLabSimulation lab;
    lab.reset();
    assert(lab.bodies().size()==2 && !lab.gravityEnabled());
    const auto labStart=lab.bodies()[0].pos;
    lab.step(0.2);
    assert((lab.bodies()[0].pos-labStart).norm()<1e-12);
    lab.setGravityEnabled(true);
    lab.step(0.1);
    assert((lab.bodies()[0].pos-labStart).norm()>1e-8);
    lab.step(350.0);
    assert(lab.capturedCount()>=1);

    const double b1=schwarzschildDeflection(5.0);
    const double b2=schwarzschildDeflection(10.0);
    const double b3=schwarzschildDeflection(30.0);
    assert(std::isfinite(b1) && b1>b2 && b2>b3);
    assert(b2>0.1 && b2<0.5);
    assert(std::abs(b3-2.0/30.0)<0.02); // asymptotic Einstein result 2 Rs/b
    auto lut=makeDeflectionLut(128);
    assert(lut.size()==128 && lut.front()>lut.back());
    std::cout << "Physics tests passed.  b=10Rs deflection=" << b2
              << " rad, relative conservation drift=" <<
                 std::abs(finalEnergy-initialEnergy)/std::max(0.001,std::abs(initialEnergy)) << "\n";
}
