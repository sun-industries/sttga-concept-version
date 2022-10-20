#include "transforms.hpp"
#include <cmath>

EcfV3 geodeticToEcf(Geodetic geodetic) {

  const double a = 6378.137;
  const double b = 6356.7523142;
  const double f = (a - b) / a;
  const double e2 = ((2 * f) - (f * f));

  double normal = a / sqrt(1 - (e2 * (sin(geodetic.latitude) * sin(geodetic.latitude))));

  double x = (normal + geodetic.height) * cos(geodetic.latitude) * cos(geodetic.longitude);
  double y = (normal + geodetic.height) * cos(geodetic.latitude) * sin(geodetic.longitude);
  double z = ((normal * (1 - e2)) + geodetic.height) * sin(geodetic.latitude);

  return {
    x,
    y,
    z,
  };
}


Topocentric topocentric(Geodetic observerGeodetic, V3 satelliteEcf) {
  // http://www.celestrak.com/columns/v02n02/
  // TS Kelso's method, except I'm using ECF frame
  // and he uses ECI.

  EcfV3 observerEcf = geodeticToEcf(observerGeodetic);

  double rx = satelliteEcf.x - observerEcf.x;
  double ry = satelliteEcf.y - observerEcf.y;
  double rz = satelliteEcf.z - observerEcf.z;

  double topS = ((sin(observerGeodetic.latitude) * cos(observerGeodetic.longitude) * rx)
      + (sin(observerGeodetic.latitude) * sin(observerGeodetic.longitude) * ry))
    - (cos(observerGeodetic.latitude) * rz);

  double topE = (-sin(observerGeodetic.longitude) * rx)
    + (cos(observerGeodetic.longitude) * ry);

  double topZ = (cos(observerGeodetic.latitude) * cos(observerGeodetic.longitude) * rx)
    + (cos(observerGeodetic.latitude) * sin(observerGeodetic.longitude) * ry)
    + (sin(observerGeodetic.latitude) * rz);

  return { topS, topE, topZ };
}

Geodetic eciToGeodetic(EciV3 eci, double gmst) {
  // http://www.celestrak.com/columns/v02n03/
  const double a = 6378.137;
  const double b = 6356.7523142;
  double R = sqrt((eci.x * eci.x) + (eci.y * eci.y));
  const double f = (a - b) / a;
  const double e2 = ((2.0 * f) - (f * f));
  const double twoPi = 2.0 * pi;

  double longitude = atan2(eci.y, eci.x) - gmst;
  while (longitude < -pi) {
    longitude += twoPi;
  }
  while (longitude > pi) {
    longitude -= twoPi;
  }

  const int kmax = 20;
  int k = 0;
  double latitude = atan2(
    eci.z,
    sqrt((eci.x * eci.x) + (eci.y * eci.y))
  );
  double C;
  while (k < kmax) {
    C = 1.0 / sqrt(1.0 - (e2 * (sin(latitude) * sin(latitude))));
    latitude = atan2(eci.z + (a * C * e2 * sin(latitude)), R);
    k += 1;
  }
  double height = (R / cos(latitude)) - (a * C);

  return { longitude, latitude, height };
}


EcfV3 eciToEcf(EciV3 eci, double gmst) {
  // ccar.colorado.edu/ASEN5070/handouts/coordsys.doc
  //
  // [X]     [C -S  0][X]
  // [Y]  =  [S  C  0][Y]
  // [Z]eci  [0  0  1][Z]ecf
  //
  //
  // Inverse:
  // [X]     [C  S  0][X]
  // [Y]  =  [-S C  0][Y]
  // [Z]ecf  [0  0  1][Z]eci

  double x = (eci.x * cos(gmst)) + (eci.y * sin(gmst));
  double y = (eci.x * (-sin(gmst))) + (eci.y * cos(gmst));
  double z = eci.z;

  return {
    x,
    y,
    z,
  };
}

Topocentric topocentric(Geodetic observerGeodetic, EcfV3 satelliteEcf) {
  // http://www.celestrak.com/columns/v02n02/
  // TS Kelso's method, except I'm using ECF frame
  // and he uses ECI.

  EcfV3 observerEcf = geodeticToEcf(observerGeodetic);

  double rx = satelliteEcf.x - observerEcf.x;
  double ry = satelliteEcf.y - observerEcf.y;
  double rz = satelliteEcf.z - observerEcf.z;

  double topS = ((sin(observerGeodetic.latitude) * cos(observerGeodetic.longitude) * rx)
      + (sin(observerGeodetic.latitude) * sin(observerGeodetic.longitude) * ry))
    - (cos(observerGeodetic.latitude) * rz);

  double topE = (-sin(observerGeodetic.longitude) * rx)
    + (cos(observerGeodetic.longitude) * ry);

  double topZ = (cos(observerGeodetic.latitude) * cos(observerGeodetic.longitude) * rx)
    + (cos(observerGeodetic.latitude) * sin(observerGeodetic.longitude) * ry)
    + (sin(observerGeodetic.latitude) * rz);

  return { topS, topE, topZ };
}

LookAngles topocentricToLookAngles(Topocentric tc) {
  double rangeSat = sqrt((tc.topS * tc.topS) + (tc.topE * tc.topE) + (tc.topZ * tc.topZ));
  double El = asin(tc.topZ / rangeSat);
  double Az = atan2(-tc.topE, tc.topS) + pi;

  return {
    Az,
    El,
    rangeSat // Range in km
  };
}

LookAngles ecfToLookAngles(Geodetic observerGeodetic, EcfV3 satelliteEcf) {
  auto topocentricCoords = topocentric(observerGeodetic, satelliteEcf);
  return topocentricToLookAngles(topocentricCoords);
}

double radiansToDegrees(double radians) {
  return radians * rad2deg;
}