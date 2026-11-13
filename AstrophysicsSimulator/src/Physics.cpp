#include "Physics.hpp"
#include <algorithm>
#include <limits>
#include <numeric>

namespace astro {
namespace {
constexpr double DISK_A = 6.5, DISK_B = 0.26; // kpc: Miyamoto-Nagai
constexpr double BULGE_A = 0.7; // kpc: Hernquist
constexpr double HALO_RC = 3.0; // kpc: cored logarithmic halo
std::mt19937 makeRng(unsigned seed) { return std::mt19937(seed); }
Vec3 rotated(Vec3 v,double i,double omega,double arg) {
    double ca=std::cos(arg),sa=std::sin(arg);
    Vec3 p={ca*v.x-sa*v.y,sa*v.x+ca*v.y,v.z};
    double ci=std::cos(i),si=std::sin(i);
    p={p.x,ci*p.y-si*p.z,si*p.y+ci*p.z};
    double co=std::cos(omega),so=std::sin(omega);
    return {co*p.x-so*p.y,so*p.x+co*p.y,p.z};
}
}

Vec3 galaxyAcceleration(Vec3 pos, const GalaxyParams& p) {
    const double R2=pos.x*pos.x+pos.y*pos.y;
    const double zTerm=std::sqrt(pos.z*pos.z+DISK_B*DISK_B);
    const double B=DISK_A+zTerm;
    const double diskD2=R2+B*B;
    const double invDiskD3=1.0/(diskD2*std::sqrt(diskD2));
    const double diskMu=G_KPC_MYR*p.diskMass;
    Vec3 a={-diskMu*pos.x*invDiskD3,
            -diskMu*pos.y*invDiskD3,
            -diskMu*B*pos.z*invDiskD3/zTerm};
    const double r2=R2+pos.z*pos.z;
    const double r=std::sqrt(std::max(1e-16,r2));
    const double bulgeD=r+BULGE_A;
    a+=pos*(-G_KPC_MYR*p.bulgeMass/(r*bulgeD*bulgeD));
    // Softened central compact mass; strong-field regime is handled separately.
    const double bhD2=r2+0.004*0.004;
    a+=pos*(-G_KPC_MYR*p.centralBlackHoleMass/(bhD2*std::sqrt(bhD2)));
    if(p.darkMatter) {
        const double v=p.darkHaloSpeedKms*KM_PER_S_TO_KPC_PER_MYR;
        a+=pos*(-v*v/(r*r+HALO_RC*HALO_RC));
    }
    return a;
}

double galaxyPotential(Vec3 pos, const GalaxyParams& p) {
    const double r=pos.norm();
    const double q=DISK_A+std::sqrt(pos.z*pos.z+DISK_B*DISK_B);
    double pot=-G_KPC_MYR*p.diskMass/std::sqrt(pos.x*pos.x+pos.y*pos.y+q*q)
               -G_KPC_MYR*p.bulgeMass/(r+BULGE_A)
               -G_KPC_MYR*p.centralBlackHoleMass/std::sqrt(r*r+0.004*0.004);
    if(p.darkMatter) {
        const double v=p.darkHaloSpeedKms*KM_PER_S_TO_KPC_PER_MYR;
        pot+=0.5*v*v*std::log(r*r+HALO_RC*HALO_RC); // absolute zero arbitrary
    }
    return pot;
}

void GalaxySimulation::reset(std::size_t count, std::uint32_t seed) {
    stars_.clear(); stars_.reserve(count); timeMyr_=0;
    accelerations_.clear();nextAccelerations_.clear();accelerationCacheValid_=false;
    auto rng=makeRng(seed);
    std::uniform_real_distribution<double> u(0.0,1.0);
    std::normal_distribution<double> gaussian(0.0,1.0);
    for(std::size_t j=0;j<count;++j) {
        const double kind=u(rng);
        double r=0,theta=2*PI*u(rng),z=0;
        if(kind<0.84) { // exponential disk (gamma distribution with shape 2)
            const double e1=std::max(1e-12,u(rng)),e2=std::max(1e-12,u(rng));
            r=std::clamp(-3.1*std::log(e1*e2),0.12,28.0);
            if(u(rng)<0.72) {
                const int arm=int(4*u(rng));
                theta=arm*PI/2.0+std::log(std::max(r,0.5)/3.5)/std::tan(13.5*PI/180.0)
                      +0.20*gaussian(rng);
            }
            z=0.15*gaussian(rng)*(1+0.04*r);
        } else if(kind<0.96) { // central bulge
            r=std::min(7.0,-1.3*std::log(std::max(1e-12,u(rng))));
            theta=2*PI*u(rng);
            z=gaussian(rng)*std::max(0.1,0.37*r);
        } else { // stellar halo tracers, not particles of dark matter
            r=5.0+33.0*std::pow(u(rng),0.7);
            theta=2*PI*u(rng);
            z=(2*u(rng)-1.0)*r;
        }
        Vec3 pos={r*std::cos(theta),r*std::sin(theta),z};
        const Vec3 acc=galaxyAcceleration({pos.x,pos.y,0},params_);
        const double radialAcc=(r>1e-6? (acc.x*pos.x+acc.y*pos.y)/r : -0.0);
        const double v=std::sqrt(std::max(0.0,-r*radialAcc));
        const double noise=(kind<0.84?0.07:(kind<0.96?0.27:0.38));
        Vec3 vel={-v*std::sin(theta),v*std::cos(theta),0};
        vel+=Vec3{v*noise*gaussian(rng),v*noise*gaussian(rng),v*noise*gaussian(rng)*0.5};
        Body s; s.pos=pos; s.vel=vel;
        double t=u(rng);
        if(t<0.07) { s.r=0.48f;s.g=0.68f;s.b=1.0f; }
        else if(t<0.30) { s.r=0.79f;s.g=0.86f;s.b=1.0f; }
        else if(t<0.79) { s.r=1.0f;s.g=0.95f;s.b=0.81f; }
        else { s.r=1.0f;s.g=0.67f;s.b=0.41f; }
        s.size=(u(rng)<0.008?4.0f:(u(rng)<0.18?2.0f:1.0f));
        stars_.push_back(s);
    }
}

void GalaxySimulation::setDarkMatter(bool on) {
    if(params_.darkMatter==on) return;
    params_.darkMatter=on;
    accelerationCacheValid_=false;
}

void GalaxySimulation::setBlackHoleMass(double massSolar) {
    params_.centralBlackHoleMass=std::max(0.0,massSolar);
    accelerationCacheValid_=false;
}

void GalaxySimulation::step(double dtMyr) {
    if(dtMyr<=0 || stars_.empty()) return;
    // Velocity-Verlet leapfrog; fixed gravitational background, no self-gravity.
    const double h=std::min(dtMyr,0.5);
    const int steps=std::max(1,int(std::ceil(dtMyr/h)));
    const double dt=dtMyr/steps;
    accelerations_.resize(stars_.size());
    nextAccelerations_.resize(stars_.size());
    if(!accelerationCacheValid_) {
#ifdef _OPENMP
#pragma omp parallel for if(stars_.size() >= 50000)
#endif
        for(std::ptrdiff_t i=0;i<std::ptrdiff_t(stars_.size());++i)
            accelerations_[std::size_t(i)]=galaxyAcceleration(stars_[std::size_t(i)].pos,params_);
        accelerationCacheValid_=true;
    }
    for(int k=0;k<steps;++k) {
#ifdef _OPENMP
#pragma omp parallel for if(stars_.size() >= 50000)
#endif
        for(std::ptrdiff_t i=0;i<std::ptrdiff_t(stars_.size());++i) {
            auto& s=stars_[std::size_t(i)];
            s.vel+=accelerations_[std::size_t(i)]*(dt*0.5);
            s.pos+=s.vel*dt;
            nextAccelerations_[std::size_t(i)]=galaxyAcceleration(s.pos,params_);
            s.vel+=nextAccelerations_[std::size_t(i)]*(dt*0.5);
        }
        accelerations_.swap(nextAccelerations_);
        timeMyr_+=dt;
    }
}

double gravitationalRadiusAU(double massSolar) {
    return G_AU_YR*std::max(0.0,massSolar)/(C_AU_YR*C_AU_YR);
}

double kerrHorizonRadiusRg(double spin) {
    const double a=std::clamp(std::abs(spin),0.0,1.0);
    return 1.0+std::sqrt(std::max(0.0,1.0-a*a));
}

double kerrIscoRadiusRg(double spin) {
    const double a=std::clamp(spin,-0.999999,0.999999);
    const double z1=1.0+std::cbrt(1.0-a*a)*(std::cbrt(1.0+a)+std::cbrt(1.0-a));
    const double z2=std::sqrt(3.0*a*a+z1*z1);
    const double root=std::sqrt(std::max(0.0,(3.0-z1)*(3.0+z1+2.0*z2)));
    return 3.0+z2-std::copysign(root,a==0.0?1.0:a);
}

double flammEmbeddingHeightRs(double radiusRs) {
    if(radiusRs<1.0) return std::numeric_limits<double>::quiet_NaN();
    return 2.0*std::sqrt(radiusRs-1.0);
}

double kerrEquatorialFrameDraggingOmega(double radiusRg,double spin) {
    const double r=std::max(kerrHorizonRadiusRg(spin),radiusRg);
    const double a=std::clamp(spin,-0.998,0.998);
    const double delta=r*r-2.0*r+a*a;
    const double denominator=(r*r+a*a)*(r*r+a*a)-a*a*delta;
    return (std::abs(denominator)>1e-15?2.0*a*r/denominator:0.0);
}

double tidalDisruptionRadiusAU(double blackHoleMassSolar,double objectMassSolar,
                               double objectRadiusSolar) {
    if(blackHoleMassSolar<=0 || objectMassSolar<=0 || objectRadiusSolar<=0) return 0.0;
    return objectRadiusSolar*SOLAR_RADIUS_AU
           *std::cbrt(blackHoleMassSolar/objectMassSolar);
}

std::vector<float> makeKerrShadowLut(std::size_t resolution,double spin,double inclination) {
    std::vector<float> result(resolution,float(1.5*std::sqrt(3.0)));
    if(resolution==0) return result;
    const double a=std::clamp(spin,-0.998,0.998);
    if(std::abs(a)<1e-5) return result;

    const double inc=std::clamp(inclination,0.025,PI-0.025);
    const double sinI=std::sin(inc),cosI=std::cos(inc);
    const double cot2=(cosI*cosI)/(sinI*sinI);
    const double absA=std::abs(a);
    const double rMin=2.0*(1.0+std::cos((2.0/3.0)*std::acos(-absA)));
    const double rMax=2.0*(1.0+std::cos((2.0/3.0)*std::acos(absA)));
    const std::size_t samples=std::max<std::size_t>(32768,resolution*32);
    std::vector<double> sum(resolution,0.0);
    std::vector<unsigned> count(resolution,0);
    auto record=[&](double alpha,double beta) {
        double angle=std::atan2(beta,alpha)/(2.0*PI);
        if(angle<0) angle+=1.0;
        const std::size_t bin=std::min(resolution-1,std::size_t(angle*resolution));
        sum[bin]+=0.5*std::hypot(alpha,beta); // convert GM/c^2 to Schwarzschild radii
        ++count[bin];
    };
    for(std::size_t j=0;j<samples;++j) {
        const double r=rMin+(rMax-rMin)*double(j)/double(samples-1);
        const double denominator=a*(1.0-r);
        const double xi=(r*r*(r-3.0)+a*a*(r+1.0))/denominator;
        const double eta=r*r*r*(4.0*a*a-r*(r-3.0)*(r-3.0))
                         /(a*a*(1.0-r)*(1.0-r));
        const double beta2=eta+a*a*cosI*cosI-xi*xi*cot2;
        if(beta2<0 || !std::isfinite(beta2)) continue;
        const double alpha=-xi/sinI,beta=std::sqrt(beta2);
        record(alpha,beta);record(alpha,-beta);
    }
    for(std::size_t j=0;j<resolution;++j)
        if(count[j]) result[j]=float(sum[j]/count[j]);

    // The analytic curve is sampled parametrically, not at uniform polar angle.
    // Cyclic interpolation closes the few bins which receive no direct sample.
    for(std::size_t j=0;j<resolution;++j) if(!count[j]) {
        std::size_t left=j,right=j;
        do left=(left+resolution-1)%resolution; while(!count[left] && left!=j);
        do right=(right+1)%resolution; while(!count[right] && right!=j);
        if(count[left] && count[right]) {
            const std::size_t dl=(j+resolution-left)%resolution;
            const std::size_t dr=(right+resolution-j)%resolution;
            result[j]=float((double(result[left])*dr+double(result[right])*dl)/double(dl+dr));
        }
    }
    return result;
}

std::vector<Vec3> schwarzschildPhotonPath(double impactRs,double extentRs,
                                          std::size_t maxSamples) {
    std::vector<Vec3> path;
    if(!(impactRs>0.0) || !(extentRs>1.0) || maxSamples<2) return path;
    path.reserve(maxSamples);

    // In units Rs=1 the exact orbit equation for an equatorial null geodesic is
    // u'' + u = 3/2 u^2, u=1/r. Integrating in decreasing polar angle launches
    // the photon from x<0 toward x>0 with the requested impact parameter.
    const double startRadius=std::hypot(extentRs,impactRs);
    double phi=std::atan2(impactRs,-extentRs);
    double u=1.0/startRadius;
    double du=std::cos(phi)/impactRs;
    constexpr double h=-0.0025;
    auto rhs=[](double value) { return 1.5*value*value-value; };

    for(std::size_t step=0;step<maxSamples;++step) {
        const double radius=1.0/u;
        if(!std::isfinite(radius) || radius<=1.0) break;
        path.push_back({radius*std::cos(phi),radius*std::sin(phi),0.0});
        if(step>20 && radius>=startRadius && std::cos(phi)>0.0) break;

        const double k1u=du,k1v=rhs(u);
        const double k2u=du+0.5*h*k1v,k2v=rhs(u+0.5*h*k1u);
        const double k3u=du+0.5*h*k2v,k3v=rhs(u+0.5*h*k2u);
        const double k4u=du+h*k3v,k4v=rhs(u+h*k3u);
        u+=(h/6.0)*(k1u+2.0*k2u+2.0*k3u+k4u);
        du+=(h/6.0)*(k1v+2.0*k2v+2.0*k3v+k4v);
        phi+=h;
        if(u<=0.0) break;
    }
    return path;
}

void GalacticCenterSimulation::setBlackHoleSpin(double spin) {
    blackHole_.dimensionlessSpin=std::clamp(spin,-0.998,0.998);
}

void GalacticCenterSimulation::reset(double massSolar,double spin) {
    blackHole_={};
    blackHole_.massSolar=std::max(1.0,massSolar);
    blackHole_.dimensionlessSpin=std::clamp(spin,-0.998,0.998);
    years_=0;capturedCount_=0;disruptedCount_=0;stars_.clear();
    std::mt19937 rng(1111);
    std::uniform_real_distribution<double> u(0.0,1.0);
    for(int j=0;j<15;++j) {
        OrbitingStar s;
        double trueAnomaly=0.0;
        if(j==0) {
            s.name="S2 (simplified)";s.semimajorAU=1000;s.eccentricity=0.884;
            s.inclination=2.35;s.longitude=1.12;s.argumentPeriapsis=0.91;
            s.massSolar=14.0;s.radiusSolar=7.0;
            s.r=0.49f;s.g=0.79f;s.b=1.0f;
        } else if(j==1) {
            // A deliberately loss-cone orbit: it reaches its tidal radius after
            // roughly 0.8 years at the default Sgr A* mass.
            s.name="TDE probe";s.semimajorAU=220;s.eccentricity=0.991;
            s.inclination=1.05;s.longitude=2.2;s.argumentPeriapsis=0.35;
            s.massSolar=8.0;s.radiusSolar=6.0;trueAnomaly=PI;
            s.r=1.0f;s.g=0.63f;s.b=0.22f;
        } else {
            s.name="Synthetic " + std::to_string(j);
            s.semimajorAU=350+7200*u(rng);s.eccentricity=0.20+0.68*u(rng);
            s.inclination=PI*u(rng);s.longitude=2*PI*u(rng);
            s.argumentPeriapsis=2*PI*u(rng);
            s.massSolar=0.7+18.0*std::pow(u(rng),2.2);
            s.radiusSolar=std::pow(s.massSolar,0.72);
            s.r=float(0.65+0.35*u(rng));s.g=float(0.6+0.4*u(rng));s.b=float(0.65+0.35*u(rng));
        }
        const double p=s.semimajorAU*(1-s.eccentricity*s.eccentricity);
        const double radius=p/(1+s.eccentricity*std::cos(trueAnomaly));
        const double speedScale=std::sqrt(G_AU_YR*blackHole_.massSolar/p);
        s.pos=rotated({radius*std::cos(trueAnomaly),radius*std::sin(trueAnomaly),0},
                      s.inclination,s.longitude,s.argumentPeriapsis);
        s.vel=rotated({-speedScale*std::sin(trueAnomaly),
                       speedScale*(s.eccentricity+std::cos(trueAnomaly)),0},
                      s.inclination,s.longitude,s.argumentPeriapsis);
        stars_.push_back(s);
    }

    // Put the complete system in its barycentric rest frame. The black hole is
    // therefore free to execute its physically small reflex motion.
    Vec3 weightedPosition,weightedVelocity;
    double totalStellarMass=0.0;
    for(const auto& s:stars_) {
        weightedPosition+=s.pos*s.massSolar;
        weightedVelocity+=s.vel*s.massSolar;
        totalStellarMass+=s.massSolar;
    }
    const double totalMass=blackHole_.massSolar+totalStellarMass;
    blackHole_.pos=weightedPosition*(-1.0/totalMass);
    blackHole_.vel=weightedVelocity*(-1.0/totalMass);
    for(auto& s:stars_) {
        s.pos+=blackHole_.pos;
        s.vel+=blackHole_.vel;
    }
    referenceOrigin_=blackHole_.pos;
}

void GalacticCenterSimulation::calculateAccelerations(const std::vector<Vec3>& x,
                                                       const std::vector<Vec3>& v,
                                                       std::vector<Vec3>& out) const {
    out.assign(stars_.size()+1,{});
    const double c2=C_AU_YR*C_AU_YR;
    const double mass=blackHole_.massSolar;
    const double mu=G_AU_YR*mass;
    const Vec3 spinAngularMomentum=blackHole_.spinAxis
        *(blackHole_.dimensionlessSpin*G_AU_YR*mass*mass/C_AU_YR);
    for(std::size_t i=0;i<stars_.size();++i) {
        if(stars_[i].state!=ObjectState::Active) continue;
        const std::size_t k=i+1;
        const Vec3 r=x[k]-x[0],relV=v[k]-v[0];
        const double r2=std::max(1e-18,r.norm2()),radius=std::sqrt(r2),r3=r2*radius;
        const Vec3 starNewton=r*(-mu/r3);
        out[k]+=starNewton;
        out[0]+=r*(G_AU_YR*stars_[i].massSolar/r3);

        // 1PN Schwarzschild correction in harmonic coordinates.
        const double rv=r.dot(relV),v2=relV.norm2();
        const Vec3 pn=(r*(4.0*mu/radius-v2)+relV*(4.0*rv))*(mu/(c2*r3));
        // 1.5PN spin-orbit (Lense-Thirring) term for a Kerr source.
        const Vec3 lt=(cross(r,relV)*(3.0*r.dot(spinAngularMomentum)/r2)
                      +cross(relV,spinAngularMomentum))*(2.0*G_AU_YR/(c2*r3));
        const Vec3 relativistic=pn+lt;
        out[k]+=relativistic;
        out[0]-=relativistic*(stars_[i].massSolar/mass);
    }

    // Direct Newtonian object-object forces are inexpensive for the 15-body
    // Galactic Centre scene and make close multiple encounters self-consistent.
    for(std::size_t i=0;i<stars_.size();++i) {
        if(stars_[i].state!=ObjectState::Active) continue;
        for(std::size_t j=i+1;j<stars_.size();++j) {
            if(stars_[j].state!=ObjectState::Active) continue;
            const Vec3 r=x[j+1]-x[i+1];
            const double r2=std::max(1e-18,r.norm2()),r3=r2*std::sqrt(r2);
            out[i+1]+=r*(G_AU_YR*stars_[j].massSolar/r3);
            out[j+1]-=r*(G_AU_YR*stars_[i].massSolar/r3);
        }
    }
}

double GalacticCenterSimulation::adaptiveStep(double requested) const {
    double h=std::min(requested,0.003);
    for(const auto& s:stars_) if(s.state==ObjectState::Active) {
        const double radius=std::max(1e-6,(s.pos-blackHole_.pos).norm());
        const double dynamical=std::sqrt(radius*radius*radius
                              /(G_AU_YR*(blackHole_.massSolar+s.massSolar)));
        h=std::min(h,0.035*dynamical);
    }
    return std::max(1e-8,h);
}

void GalacticCenterSimulation::accrete(OrbitingStar& object,double fraction,
                                        ObjectState finalState) {
    const double dm=std::clamp(fraction,0.0,1.0)*object.massSolar;
    const double oldMass=blackHole_.massSolar,newMass=oldMass+dm;
    const Vec3 relativePosition=object.pos-blackHole_.pos;
    const Vec3 relativeVelocity=object.vel-blackHole_.vel;
    const double oldJ=blackHole_.dimensionlessSpin*G_AU_YR*oldMass*oldMass/C_AU_YR;
    const double addedJ=dm*cross(relativePosition,relativeVelocity).dot(blackHole_.spinAxis);
    blackHole_.pos=(blackHole_.pos*oldMass+object.pos*dm)/newMass;
    blackHole_.vel=(blackHole_.vel*oldMass+object.vel*dm)/newMass;
    blackHole_.massSolar=newMass;
    blackHole_.dimensionlessSpin=std::clamp((oldJ+addedJ)*C_AU_YR
                                    /(G_AU_YR*newMass*newMass),-0.998,0.998);
    blackHole_.accretedMassSolar+=dm;
    object.state=finalState;
    object.pos=blackHole_.pos;object.vel={};
    if(finalState==ObjectState::Captured) ++capturedCount_;
    else ++disruptedCount_;
}

void GalacticCenterSimulation::handleInteractions() {
    for(auto& s:stars_) {
        if(s.state!=ObjectState::Active) continue;
        const Vec3 relative=s.pos-blackHole_.pos;
        const double radius=relative.norm();
        const Vec3 angularMomentum=cross(relative,s.vel-blackHole_.vel);
        const Vec3 spinDirection=blackHole_.spinAxis
                                 *(blackHole_.dimensionlessSpin<0?-1.0:1.0);
        const bool prograde=angularMomentum.dot(spinDirection)>=0;
        const double orbitSpin=(prograde?1.0:-1.0)*std::abs(blackHole_.dimensionlessSpin);
        const double marginallyBoundRg=2.0-orbitSpin+2.0*std::sqrt(1.0-orbitSpin);
        const double captureRadius=marginallyBoundRg*gravitationalRadiusAU(blackHole_.massSolar);
        const double tidalRadius=tidalDisruptionRadiusAU(blackHole_.massSolar,
                                                         s.massSolar,s.radiusSolar);
        if(tidalRadius>captureRadius && radius<=tidalRadius)
            accrete(s,0.5,ObjectState::TidallyDisrupted);
        else if(radius<=captureRadius)
            accrete(s,1.0,ObjectState::Captured);
    }
}

void GalacticCenterSimulation::step(double dtYears) {
    if(dtYears<=0 || activeObjectCount()==0) return;
    const std::size_t n=stars_.size()+1;
    std::vector<Vec3> x(n),v(n),tmpX(n),tmpV(n);
    std::vector<Vec3> k1x(n),k1v(n),k2x(n),k2v(n),k3x(n),k3v(n),k4x(n),k4v(n);
    double remaining=dtYears;
    while(remaining>1e-12) {
        const double h=std::min(remaining,adaptiveStep(remaining));
        x[0]=blackHole_.pos;v[0]=blackHole_.vel;
        for(std::size_t i=0;i<stars_.size();++i) {
            x[i+1]=stars_[i].pos;
            v[i+1]=(stars_[i].state==ObjectState::Active?stars_[i].vel:Vec3{});
        }
        k1x=v;calculateAccelerations(x,v,k1v);
        for(std::size_t i=0;i<n;++i) {
            tmpX[i]=x[i]+k1x[i]*(0.5*h);tmpV[i]=v[i]+k1v[i]*(0.5*h);
        }
        k2x=tmpV;calculateAccelerations(tmpX,tmpV,k2v);
        for(std::size_t i=0;i<n;++i) {
            tmpX[i]=x[i]+k2x[i]*(0.5*h);tmpV[i]=v[i]+k2v[i]*(0.5*h);
        }
        k3x=tmpV;calculateAccelerations(tmpX,tmpV,k3v);
        for(std::size_t i=0;i<n;++i) {
            tmpX[i]=x[i]+k3x[i]*h;tmpV[i]=v[i]+k3v[i]*h;
        }
        k4x=tmpV;calculateAccelerations(tmpX,tmpV,k4v);
        blackHole_.pos+= (k1x[0]+2*k2x[0]+2*k3x[0]+k4x[0])*(h/6.0);
        blackHole_.vel+= (k1v[0]+2*k2v[0]+2*k3v[0]+k4v[0])*(h/6.0);
        for(std::size_t i=0;i<stars_.size();++i) if(stars_[i].state==ObjectState::Active) {
            const std::size_t k=i+1;
            stars_[i].pos+=(k1x[k]+2*k2x[k]+2*k3x[k]+k4x[k])*(h/6.0);
            stars_[i].vel+=(k1v[k]+2*k2v[k]+2*k3v[k]+k4v[k])*(h/6.0);
        }
        years_+=h;remaining-=h;
        handleInteractions();
    }
}

std::size_t GalacticCenterSimulation::activeObjectCount() const {
    return std::size_t(std::count_if(stars_.begin(),stars_.end(),[](const OrbitingStar& s) {
        return s.state==ObjectState::Active;
    }));
}

Vec3 GalacticCenterSimulation::totalMomentum() const {
    Vec3 result=blackHole_.vel*blackHole_.massSolar;
    for(const auto& s:stars_) if(s.state==ObjectState::Active)
        result+=s.vel*s.massSolar;
    return result;
}

void PhysicsLabSimulation::reset() {
    time_=0.0;capturedCount_=0;gravityEnabled_=false;bodies_.clear();
    // Normalized counterparts of the two solar-mass objects in the reference
    // scene: 4e11 m from a Sgr A*-scale hole with Rs=1.269e10 m.
    constexpr double distanceRs=31.521;
    constexpr double solarToBlackHole=2.329e-7;
    constexpr double objectRadiusRs=3.152;
    bodies_.push_back({"Yellow object",{distanceRs,0.0,0.0},{},
                       solarToBlackHole,objectRadiusRs,solarToBlackHole,
                       1.0f,1.0f,0.0f,true});
    bodies_.push_back({"Red object",{0.0,distanceRs,0.0},{},
                       solarToBlackHole,objectRadiusRs,solarToBlackHole,
                       1.0f,0.0f,0.0f,true});
}

void PhysicsLabSimulation::respawn(PhysicsLabBody& body,std::size_t index) {
    constexpr double distanceRs=31.521;
    body.pos=(index%2==0?Vec3{distanceRs,0.0,0.0}:Vec3{0.0,distanceRs,0.0});
    body.vel={};
    body.active=true;
}

void PhysicsLabSimulation::step(double dt) {
    if(dt<=0.0 || bodies_.empty()) return;
    time_+=dt;
    // The reference demo freezes its objects when gravity is disabled rather
    // than continuing with inertial motion. Preserve that observable behavior.
    if(!gravityEnabled_) return;
    const int substeps=std::max(1,int(std::ceil(dt/0.02)));
    const double h=dt/substeps;
    std::vector<Vec3> acceleration(bodies_.size()),next(bodies_.size());
    auto evaluate=[&](std::vector<Vec3>& out) {
        std::fill(out.begin(),out.end(),Vec3{});
        for(std::size_t i=0;i<bodies_.size();++i) {
            if(!bodies_[i].active) continue;
            const Vec3 r=bodies_[i].pos;
            const double r2=std::max(0.04,r.norm2()),r3=r2*std::sqrt(r2);
            out[i]+=r*(-0.5/r3); // G*M=1/2 when Rs=c=1
        }
        for(std::size_t i=0;i<bodies_.size();++i) for(std::size_t j=i+1;j<bodies_.size();++j) {
            if(!bodies_[i].active || !bodies_[j].active) continue;
            const Vec3 r=bodies_[j].pos-bodies_[i].pos;
            const double r2=std::max(0.04,r.norm2()),r3=r2*std::sqrt(r2);
            out[i]+=r*(0.5*bodies_[j].mass/r3);
            out[j]-=r*(0.5*bodies_[i].mass/r3);
        }
    };
    evaluate(acceleration);
    for(int substep=0;substep<substeps;++substep) {
        for(std::size_t i=0;i<bodies_.size();++i) if(bodies_[i].active) {
            bodies_[i].vel+=acceleration[i]*(0.5*h);
            bodies_[i].pos+=bodies_[i].vel*h;
        }
        evaluate(next);
        for(std::size_t i=0;i<bodies_.size();++i) if(bodies_[i].active)
            bodies_[i].vel+=next[i]*(0.5*h);
        acceleration.swap(next);
        for(std::size_t i=0;i<bodies_.size();++i) if(bodies_[i].active
                                                    && bodies_[i].pos.norm()<=1.0) {
            ++capturedCount_;
            respawn(bodies_[i],i);
        }
    }
}

std::vector<Vec3> GalacticCenterSimulation::referenceOrbit(const OrbitingStar& s,int samples) const {
    std::vector<Vec3> out;out.reserve(std::max(2,samples));
    const double a=s.semimajorAU,e=s.eccentricity,p=a*(1-e*e);
    for(int j=0;j<std::max(2,samples);++j) {
        const double f=2*PI*j/double(std::max(2,samples)-1);
        const double r=p/(1+e*std::cos(f));
        out.push_back(referenceOrigin_+rotated({r*std::cos(f),r*std::sin(f),0},
                                              s.inclination,s.longitude,s.argumentPeriapsis));
    }
    return out;
}

double schwarzschildDeflection(double b) {
    const double critical=1.5*std::sqrt(3.0);
    if(!(b>critical)) return std::numeric_limits<double>::infinity();
    // Find outer turning point x=r_min/Rs, with b=x/sqrt(1-1/x).
    double low=1.5, high=std::max(3.0,2*b);
    for(int k=0;k<64;++k) {
        const double x=(low+high)*0.5;
        if(x/std::sqrt(1.0-1.0/x)>b) high=x;
        else low=x;
    }
    const double r0=0.5*(low+high);
    const double br=b/r0;
    // Substitution z=1-t^2 regularizes square-root singularity at turning point.
    constexpr int n=512;
    auto f=[&](double t) {
        if(t<=1e-9) return 2.0/std::sqrt(std::max(1e-14,2.0-3.0/r0));
        const double z=1.0-t*t;
        const double term=1.0-br*br*z*z+(br*br/r0)*z*z*z;
        return 2.0*t*br/std::sqrt(std::max(1e-20,term));
    };
    double sum=f(0)+f(1);
    for(int j=1;j<n;++j) sum+=(j%2?4.0:2.0)*f(double(j)/n);
    const double half=(sum/(3.0*n));
    return std::max(0.0,2.0*half-PI);
}

std::vector<float> makeDeflectionLut(std::size_t resolution,double bMax) {
    const double bc=1.5*std::sqrt(3.0);
    std::vector<float> lut(resolution,0);
    for(std::size_t j=0;j<resolution;++j) {
        const double t=double(j)/std::max<std::size_t>(1,resolution-1);
        const double b=bc+0.001+(bMax-bc-0.001)*t;
        lut[j]=float(std::min(80.0,schwarzschildDeflection(b)));
    }
    return lut;
}
}
