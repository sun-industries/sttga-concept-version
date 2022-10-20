// Based on https://github.com/nsat/jspredict/
#include <cmath>
#include "transforms.hpp"

#define pi 3.14159265358979323846

namespace solar {

    const double ms2day = 1000 * 60 * 60 * 24; // milliseconds to day
    const double deg2rad = pi / 180;
    const double xkmper = 6.378137E3; // earth radius (km) wgs84
    const double astro_unit = 1.49597870691E8; // Astronomical unit - km (IAU 76)
    const double solar_radius = 6.96000E5; // solar radius - km (IAU 76)

    struct V4 {
        double x;
        double y;
        double z;
        double w;
    };

    struct EclipseStatus {
        double depth;
        bool eclipsed;
    };

    double deltaET(double year) {
        return 26.465 + 0.747622 * (year - 1950.0) + 1.886913 * sin((2.0 * pi) * (year - 1975.0) / 33.0);
    }

    double magnitude(V3 v) {
        return sqrt(pow(v.x, 2) + pow(v.y, 2) + pow(v.z, 2));
    }

    V4 calculateSolarPosition(double jday) {

        double mjd = jday - 2415020.0; // DJD?
        double year = 1900.0 + mjd / 365.25;
        double T = (mjd + deltaET(year) / (ms2day / 1000)) / 36525.0;
        double M = deg2rad * fmod(358.47583 + fmod(35999.04975 * T, 360) - (0.000150 + 0.0000033 * T) * pow(T, 2), 360);
        double L = deg2rad * fmod(279.69668 + fmod(36000.76892 * T, 360) + 0.0003025 * pow(T, 2), 360);
        double e = 0.01675104 - (0.0000418 + 0.000000126 * T) * T;
        double C = deg2rad * ((1.919460 - (0.004789 + 0.000014 * T) * T) * sin(M) + (0.020094 - 0.000100 * T) * sin(2 * M) + 0.000293 * sin(3 * M));
        double O = deg2rad * fmod(259.18 - 1934.142 * T, 360.0);
        double Lsa = fmod(L + C - deg2rad * (0.00569 - 0.00479 * sin(O)), 2 * pi);
        double nu = fmod(M + C, 2 * pi);
        double R = 1.0000002 * (1 - pow(e, 2)) / (1 + e * cos(nu));
        double eps = deg2rad * (23.452294 - (0.0130125 + (0.00000164 - 0.000000503 * T) * T) * T + 0.00256 * cos(O));
        R = astro_unit * R;

        return {
            R * cos(Lsa),
            R * sin(Lsa) * cos(eps),
            R * sin(Lsa) * sin(eps),
            R
        };
    }

    EclipseStatus satEclipsed(EciV3 pos, V4 sol) {
        double sd_earth = asin(xkmper / magnitude(pos));

        // rho = sol - pos
        V4 rho;
        rho.x = sol.x - pos.x;
        rho.y = sol.y - pos.y;
        rho.z = sol.z - pos.z;
        rho.w = magnitude({
            rho.x, rho.y, rho.z
        });
        
        double sd_sun = asin(solar_radius / rho.w);

        // earth = -1 * pos
        V3 earth;
        earth.x =  -1 * pos.x;
        earth.x =  -1 * pos.y;
        earth.x =  -1 * pos.z;

        // delta = angle(sol, earth)
        double dot = (sol.x * earth.x + sol.y * earth.y + sol.z * earth.z);
        double delta = acos(dot / (magnitude((V3){sol.x, sol.y, sol.z}) * magnitude(earth)));

        double eclipseDepth = sd_earth - sd_sun - delta;
        bool eclipsed;
        if(sd_earth < sd_sun) {
        eclipsed = false;
        } else {
        eclipsed = eclipseDepth >= 0;
        }
        return {
        eclipseDepth,
        eclipsed
        };
    }

}