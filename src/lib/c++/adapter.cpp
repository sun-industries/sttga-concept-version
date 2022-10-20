/**
 * @file adapter.cpp
 * @author Rodrigo González (rgonzalez@sun.industries)
 * @brief
 * @version 0.1
 * @date 2022-08-21
 *
 * @copyright Copyright (c) 2022
 *
 */
//////////////////////////////////////////////////

#include <string>
#include <sstream>
#include <regex>
#include <cmath>
//#ifdef __EMSCRIPTEN__
//#include <emscripten.h>
#include <emscripten/bind.h>


#include "SGP4.cpp"
#include "transforms.cpp"
#include "transforms.hpp"
#include "solar.cpp"

using namespace emscripten;

struct ObserveResult {
    EciV3 eci;
    Geodetic geodetic; // this is in grades
    LookAngles lookAngles;
    solar::EclipseStatus eclipse_status;
};


std::vector<elsetrec> satrecs;


extern "C" elsetrec twoline2satrec(std::string tleline1, std::string tleline2)
{
    char *longstr1 = new char[130];
    char *longstr2 = new char[130];
    strcpy(longstr1, tleline1.c_str());
    strcpy(longstr2, tleline2.c_str());
    double startmfe, stopmfe, deltamin;
    elsetrec satrec;
    SGP4Funcs::twoline2rv(
        longstr1,
        longstr2,
        typerun,
        typeinput,
        opsmode,
        gravconsttype::wgs84,
        startmfe,
        stopmfe,
        deltamin,
        satrec);
    // call the propagator to get the initial state vector value
    double r0[3], v0[3];
    SGP4Funcs::sgp4(satrec, 0.0, r0, v0);
    delete [] longstr1;
    delete [] longstr2;
    return satrec;
};

extern "C" PosVel predict(elsetrec& satrec, int year, int  mon, int day, int hr, int minute, int sec) {

    double jd, jdFrac;
    SGP4Funcs::jday_SGP4(year, mon, day, hr, minute, sec, jd, jdFrac);
    
    double m = (jd - satrec.jdsatepoch) * MINUTES_PER_DAY
        + (jdFrac - satrec.jdsatepochF) * MINUTES_PER_DAY;

    double pos[3];
    double vel[3];
    SGP4Funcs::sgp4(satrec, m, pos, vel);

    EciV3 pos_v3;
    pos_v3.x = pos[0];
    pos_v3.y = pos[1];
    pos_v3.z = pos[2];

    V3 vel_v3;
    vel_v3.x = vel[0];
    vel_v3.y = vel[1];
    vel_v3.z = vel[2];

    PosVel posvel;
    posvel.pos = pos_v3;
    posvel.vel = vel_v3;

    return posvel;
}

extern "C" std::vector<PosVel> predict2(std::vector<elsetrec>& satrecs, 
int year, int  mon, int day, int hr, int minute, int sec)
{
    std::vector<PosVel> posvels;
    double jd, jdFrac;
    SGP4Funcs::jday_SGP4(year, mon, day, hr, minute, sec, jd, jdFrac);

    double pos[3];
    double vel[3];
    double m;
    EciV3 pos_v3;
    V3 vel_v3;
    PosVel posvel;

    for(elsetrec& satrec : satrecs) {

        m = (jd - satrec.jdsatepoch) * MINUTES_PER_DAY
        + (jdFrac - satrec.jdsatepochF) * MINUTES_PER_DAY;

        SGP4Funcs::sgp4(satrec, m, pos, vel);

        pos_v3.x = pos[0];
        pos_v3.y = pos[1];
        pos_v3.z = pos[2];

        vel_v3.x = vel[0];
        vel_v3.y = vel[1];
        vel_v3.z = vel[2];

        posvel.pos = pos_v3;
        posvel.vel = vel_v3;

        posvels.push_back(posvel);
    }
    return posvels;
}





extern "C" void init(std::string tle_string)
{
    satrecs.clear();

    std::stringstream tle_stream;
    tle_stream.str(tle_string);
    std::string tle_line;

    std::string line1;
    std::string line2;
    bool has_more;
    do {
        has_more = (bool) std::getline(tle_stream, line1, '\n');
        if(has_more) {
            has_more = (bool) std::getline(tle_stream, line2, '\n');
            if(has_more) {
                elsetrec satrec = twoline2satrec(line1, line2);
                satrecs.push_back(satrec);
            }
        }
    } while(has_more);
}

extern "C" std::vector<elsetrec> getSatrecs() {
    return satrecs;
}

extern "C" std::vector<ObserveResult> observe_at(Geodetic observer, int year, int  mon, int day, int hr, int minute, int sec)
{
    
    std::vector<ObserveResult> results;

    double jd, jdFrac;
    SGP4Funcs::jday_SGP4(year, mon, day, hr, minute, sec, jd, jdFrac);
    double gmst = SGP4Funcs::gstime_SGP4(jd + jdFrac);

    double pos[3];
    double vel[3];
    double m;
    solar::EclipseStatus eclipse_status;
    EciV3 pos_eci;

    solar::V4 solarVector = solar::calculateSolarPosition(jd + jdFrac);

    for(elsetrec& satrec : satrecs) {

        m = (jd - satrec.jdsatepoch) * MINUTES_PER_DAY
        + (jdFrac - satrec.jdsatepochF) * MINUTES_PER_DAY;

        SGP4Funcs::sgp4(satrec, m, pos, vel);

        pos_eci.x = pos[0];
        pos_eci.y = pos[1];
        pos_eci.z = pos[2];

        auto geodetic = eciToGeodetic(pos_eci, gmst);
        geodetic.latitude = radiansToDegrees(geodetic.latitude);
        geodetic.longitude = radiansToDegrees(geodetic.longitude);
        geodetic.height = geodetic.height / earthRadius;

        eclipse_status = satEclipsed(pos_eci, solarVector);

        // has ground observer
        EcfV3 pos_ecf = eciToEcf(pos_eci, gmst);
        LookAngles lookAngles = ecfToLookAngles(observer, pos_ecf);

        results.push_back({
            pos_eci,
            geodetic,
            lookAngles,
            eclipse_status
        });

    }
    return results;
}

extern "C" std::vector<ObserveResult> at(int year, int  mon, int day, int hr, int minute, int sec)
{
    
    std::vector<ObserveResult> results;

    double jd, jdFrac;
    SGP4Funcs::jday_SGP4(year, mon, day, hr, minute, sec, jd, jdFrac);
    double gmst = SGP4Funcs::gstime_SGP4(jd + jdFrac);

    double pos[3];
    double vel[3];
    double m;
    solar::EclipseStatus eclipse_status;
    EciV3 pos_eci;
    ObserveResult obsRes;

    solar::V4 solarVector = solar::calculateSolarPosition(jd + jdFrac);

    for(elsetrec& satrec : satrecs) {

        m = (jd - satrec.jdsatepoch) * MINUTES_PER_DAY
        + (jdFrac - satrec.jdsatepochF) * MINUTES_PER_DAY;

        SGP4Funcs::sgp4(satrec, m, pos, vel);

        pos_eci.x = pos[0];
        pos_eci.y = pos[1];
        pos_eci.z = pos[2];

        auto geodetic = eciToGeodetic(pos_eci, gmst);
        geodetic.latitude = radiansToDegrees(geodetic.latitude);
        geodetic.longitude = radiansToDegrees(geodetic.longitude);
        geodetic.height = geodetic.height / earthRadius;

        eclipse_status = satEclipsed(pos_eci, solarVector);
        obsRes.eci = pos_eci;
        obsRes.geodetic = geodetic;
        obsRes.eclipse_status = eclipse_status;

        results.push_back(obsRes);

    }
    return results;
}

EMSCRIPTEN_BINDINGS(my_elsetrec)
{
    enum_<gravconsttype>("gravconsttype")
        .value("wgs72old", wgs72old)
        .value("wgs72", wgs72)
        .value("wgs84", wgs84);

    value_object<elsetrec>("elsetrec")
        .field("satnum", &elsetrec::satnum)
        .field("epochyr", &elsetrec::epochyr)
        .field("epochtynumrev", &elsetrec::epochtynumrev)
        .field("error", &elsetrec::error)
        .field("operationmode", &elsetrec::operationmode)
        .field("init", &elsetrec::init)
        .field("method", &elsetrec::method)
        .field("isimp", &elsetrec::isimp)
        .field("aycof", &elsetrec::aycof)
        .field("con41", &elsetrec::con41)
        .field("cc1", &elsetrec::cc1)
        .field("cc4", &elsetrec::cc4)
        .field("cc5", &elsetrec::cc5)
        .field("d2", &elsetrec::d2)
        .field("d3", &elsetrec::d3)
        .field("d4", &elsetrec::d4)
        .field("delmo", &elsetrec::delmo)
        .field("eta", &elsetrec::eta)
        .field("argpdot", &elsetrec::argpdot)
        .field("omgcof", &elsetrec::omgcof)
        .field("sinmao", &elsetrec::sinmao)
        .field("t", &elsetrec::t)
        .field("t2cof", &elsetrec::t2cof)
        .field("t3cof", &elsetrec::t3cof)
        .field("t4cof", &elsetrec::t4cof)
        .field("t5cof", &elsetrec::t5cof)
        .field("x1mth2", &elsetrec::x1mth2)
        .field("x7thm1", &elsetrec::x7thm1)
        .field("mdot", &elsetrec::mdot)
        .field("nodedot", &elsetrec::nodedot)
        .field("xlcof", &elsetrec::xlcof)
        .field("xmcof", &elsetrec::xmcof)
        .field("nodecf", &elsetrec::nodecf)
        .field("irez", &elsetrec::irez)
        .field("d2201", &elsetrec::d2201)
        .field("d2211", &elsetrec::d2211)
        .field("d3210", &elsetrec::d3210)
        .field("d3222", &elsetrec::d3222)
        .field("d4410", &elsetrec::d4410)
        .field("d4422", &elsetrec::d4422)
        .field("d5220", &elsetrec::d5220)
        .field("d5232", &elsetrec::d5232)
        .field("d5421", &elsetrec::d5421)
        .field("d5433", &elsetrec::d5433)
        .field("dedt", &elsetrec::dedt)
        .field("del1", &elsetrec::del1)
        .field("del2", &elsetrec::del2)
        .field("del3", &elsetrec::del3)
        .field("didt", &elsetrec::didt)
        .field("dmdt", &elsetrec::dmdt)
        .field("dnodt", &elsetrec::dnodt)
        .field("domdt", &elsetrec::domdt)
        .field("e3", &elsetrec::e3)
        .field("ee2", &elsetrec::ee2)
        .field("peo", &elsetrec::peo)
        .field("pgho", &elsetrec::pgho)
        .field("pho", &elsetrec::pho)
        .field("pinco", &elsetrec::pinco)
        .field("plo", &elsetrec::plo)
        .field("se2", &elsetrec::se2)
        .field("se3", &elsetrec::se3)
        .field("sgh2", &elsetrec::sgh2)
        .field("sgh3", &elsetrec::sgh3)
        .field("sgh4", &elsetrec::sgh4)
        .field("sh2", &elsetrec::sh2)
        .field("sh3", &elsetrec::sh3)
        .field("si2", &elsetrec::si2)
        .field("si3", &elsetrec::si3)
        .field("sl2", &elsetrec::sl2)
        .field("sl3", &elsetrec::sl3)
        .field("sl4", &elsetrec::sl4)
        .field("gsto", &elsetrec::gsto)
        .field("xfact", &elsetrec::xfact)
        .field("xgh2", &elsetrec::xgh2)
        .field("xgh3", &elsetrec::xgh3)
        .field("xgh4", &elsetrec::xgh4)
        .field("xh2", &elsetrec::xh2)
        .field("xh3", &elsetrec::xh3)
        .field("xi2", &elsetrec::xi2)
        .field("xi3", &elsetrec::xi3)
        .field("xl2", &elsetrec::xl2)
        .field("xl3", &elsetrec::xl3)
        .field("xl4", &elsetrec::xl4)
        .field("xlamo", &elsetrec::xlamo)
        .field("zmol", &elsetrec::zmol)
        .field("zmos", &elsetrec::zmos)
        .field("atime", &elsetrec::atime)
        .field("xli", &elsetrec::xli)
        .field("xni", &elsetrec::xni)
        .field("a", &elsetrec::a)
        .field("altp", &elsetrec::altp)
        .field("alta", &elsetrec::alta)
        .field("epochdays", &elsetrec::epochdays)
        .field("jdsatepoch", &elsetrec::jdsatepoch)
        .field("jdsatepochF", &elsetrec::jdsatepochF)
        .field("nddot", &elsetrec::nddot)
        .field("ndot", &elsetrec::ndot)
        .field("bstar", &elsetrec::bstar)
        .field("rcse", &elsetrec::rcse)
        .field("inclo", &elsetrec::inclo)
        .field("nodeo", &elsetrec::nodeo)
        .field("ecco", &elsetrec::ecco)
        .field("argpo", &elsetrec::argpo)
        .field("mo", &elsetrec::mo)
        .field("no_kozai", &elsetrec::no_kozai)
        .field("classification", &elsetrec::classification)
        .field("intldesg", &elsetrec::intldesg)
        .field("ephtype", &elsetrec::ephtype)
        .field("elnum", &elsetrec::elnum)
        .field("revnum", &elsetrec::revnum)
        .field("no_unkozai", &elsetrec::no_unkozai)
        .field("am", &elsetrec::am)
        .field("em", &elsetrec::em)
        .field("im", &elsetrec::im)
        .field("Om", &elsetrec::Om)
        .field("om", &elsetrec::om)
        .field("mm", &elsetrec::mm)
        .field("nm", &elsetrec::nm)
        .field("tumin", &elsetrec::tumin)
        .field("mus", &elsetrec::mus)
        .field("radiusearthkm", &elsetrec::radiusearthkm)
        .field("xke", &elsetrec::xke)
        .field("j2", &elsetrec::j2)
        .field("j3", &elsetrec::j3)
        .field("j4", &elsetrec::j4)
        .field("j3oj2", &elsetrec::j3oj2)
        .field("dia_mm", &elsetrec::dia_mm)
        .field("period_sec", &elsetrec::period_sec)
        .field("active", &elsetrec::active)
        .field("not_orbital", &elsetrec::not_orbital)
        .field("rcs_m2", &elsetrec::rcs_m2);

    // Register std::array<int, 2> because ArrayInStruct::field is interpreted as such
    value_array<std::array<char, 6>>("char_6") // satnum
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>())
        .element(emscripten::index<3>())
        .element(emscripten::index<4>())
        .element(emscripten::index<5>());

    value_array<std::array<char, 11>>("char_11") // intldesg
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>())
        .element(emscripten::index<3>())
        .element(emscripten::index<4>())
        .element(emscripten::index<5>())
        .element(emscripten::index<6>())
        .element(emscripten::index<7>())
        .element(emscripten::index<8>())
        .element(emscripten::index<9>())
        .element(emscripten::index<10>());

    value_array<std::array<char, 16>>("char_16")
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>())
        .element(emscripten::index<3>())
        .element(emscripten::index<4>())
        .element(emscripten::index<5>())
        .element(emscripten::index<6>())
        .element(emscripten::index<7>())
        .element(emscripten::index<8>())
        .element(emscripten::index<9>())
        .element(emscripten::index<10>())
        .element(emscripten::index<11>())
        .element(emscripten::index<12>())
        .element(emscripten::index<13>())
        .element(emscripten::index<14>())
        .element(emscripten::index<15>());

    value_object<V3>("V3")
        .field("x", &V3::x)
        .field("y", &V3::y)
        .field("z", &V3::z);

    value_object<EciV3>("EciV3")
        .field("x", &EciV3::x)
        .field("y", &EciV3::y)
        .field("z", &EciV3::z);

    value_object<EcfV3>("EcfV3")
        .field("x", &EcfV3::x)
        .field("y", &EcfV3::y)
        .field("z", &EcfV3::z);

    value_object<PosVel>("PosVel")
        .field("pos", &PosVel::pos)
        .field("vel", &PosVel::vel);

    value_object<Geodetic>("Geodetic")
        .field("longitude", &Geodetic::longitude)
        .field("latitude", &Geodetic::latitude)
        .field("height", &Geodetic::height);

    value_object<Topocentric>("Topocentric")
        .field("topS", &Topocentric::topS)
        .field("topE", &Topocentric::topE)
        .field("topZ", &Topocentric::topZ);

    value_object<LookAngles>("LookAngles")
        .field("azimuth", &LookAngles::azimuth)
        .field("elevation", &LookAngles::elevation)
        .field("rangeSat", &LookAngles::rangeSat);

    value_object<solar::EclipseStatus>("EclipseStatus")
        .field("depth", &solar::EclipseStatus::depth)
        .field("eclipsed", &solar::EclipseStatus::eclipsed);

    value_object<ObserveResult>("ObserveResult")
        .field("eci", &ObserveResult::eci)
        .field("geodetic", &ObserveResult::geodetic)
        .field("lookAngles", &ObserveResult::lookAngles)
        .field("eclipse_status", &ObserveResult::eclipse_status);

    value_array<std::array<double, 3>>("double_3")
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>());

    value_object<Tle>("Tle")
        .field("line1", &Tle::line1)
        .field("line2", &Tle::line2);

    register_vector<Tle>("vector<Tle>");
    register_vector<elsetrec>("vector<elsetrec>");
    register_vector<PosVel>("vector<PosVel>");
    register_vector<Geodetic>("vector<Geodetic>");
    register_vector<ObserveResult>("vector<ObserveResult>");

    function("twoline2satrec", &twoline2satrec);//, allow_raw_pointers());
    function("predict", &predict);//, allow_raw_pointers());
    function("predict2", &predict2);

    function("init", &init);
    function("gstime", &SGP4Funcs::gstime_SGP4);
    //function("jday_SGP4", &SGP4Funcs::jday_SGP4);
    function("observe_at", &observe_at);
    function("at", &at);

    function("geodeticToEcf", &geodeticToEcf);
    //function("topocentric", &topocentric);
    function("eciToGeodetic", &eciToGeodetic);
    function("eciToEcf", &eciToEcf);
    function("topocentricToLookAngles", &topocentricToLookAngles);
    function("ecfToLookAngles", &ecfToLookAngles);
    function("getSatrecs", &getSatrecs);
    
}

// extern "C"

//#endif