import * as satellite from 'satellite.js';
import loadWASM from '$lib/c++/SGP4';

const EARTH_RADIUS_KM = 6378.137; // km
const TIME_STEP = 1 * 1000; // per frame
const TIME_EVAL = 1000;
const MINUTES_PER_DAY = 1440.0;

const COLOR_INDEX_STARLINK = 1;
const COLOR_INDEX_ONEWEB = 2;

const RAD_TO_DEG_FACTOR = 180 / Math.PI;

let SGP4;
let satData = [];
let time = new Date()
let timeMode = 'STOP';
let sleepMs = 1000;
let satrecs;

loadWASM().then((_SGP4) => {
    const time = new Date();
    const UTCFullYear = time.getUTCFullYear();
    const UTCMonth = time.getUTCMonth();
    const UTCDate = time.getUTCDate();
    const UTCHours = time.getUTCHours();
    const UTCMinutes = time.getUTCMinutes();
    const UTCSeconds = time.getUTCSeconds();
    SGP4 = _SGP4;
    const datasource = 'https://www.celestrak.com/norad/elements/active.txt'
    fetch(`https://api.allorigins.win/raw?url=${datasource}`) // https://www.celestrak.com/norad/elements/active.txt : ./src/assets/starlink.txt
        .then((r) => r.text())
        .then((rawData) => {
            const tleData = rawData
                .replace(/\r/g, "")
                .split(/\n(?=[^12])/)
                .filter((d) => d)
                .map((tle) => tle.split("\n"));
            satData = tleData
                .filter(([name, ...tle]) => {
                    return /^(STARLINK|ONEWEB)/.test(name.trim().replace(/^0 /, ""))
                })
                .map(([name, ...tle]) => ({
                    satrec: SGP4.twoline2satrec(tle[0], tle[1]),//satellite.twoline2satrec(tle[0], tle[1]),
                    name: name.trim().replace(/^0 /, ""),
                    colorIndex: /^STARLINK/.test(name.trim()) ? COLOR_INDEX_STARLINK : COLOR_INDEX_ONEWEB,
                    lat: null,
                    lng: null,
                    alt: null
                }))
                // exclude those that can't be propagated
                .filter((d) => {
                    return !!SGP4.predict(d.satrec, UTCFullYear, UTCMonth + 1, UTCDate, UTCHours, UTCMinutes, UTCSeconds).pos; //satellite.propagate(d.satrec, new Date()).position)
                })
        }).then(() => {
            //console.log('satData', satData)
            postMessage({
                type: 'tleLoaded'
            });
            requestDataFrame()
        });
    
});

self.onmessage = (event) => {
    /*if(event.data.type === 'requestDataFrame') {
        requestDataFrame(event.data.time);
    }*/
    if(event.data.type === 'setTimeMode') {
        if(timeMode === 'STOP' && event.data.timeMode !== 'STOP') {
            timeMode = event.data.timeMode
            requestDataFrame();
        } else {
            timeMode = event.data.timeMode
        }
    } else if(event.data.type === 'startPrediction') {
        const {name, from, date, mode} = event.data;
        requestDataPrediction(name, from, date, mode);
    }
}

function requestDataPrediction(name, from, date, mode) {
    const deltaTime = 10000; // 10s
    timeMode = 'PREDICTION';
    const [year, month, day] = date.split('-');
    let time = new Date(0);
    time.setUTCFullYear(parseInt(year));
    time.setUTCMonth(parseInt(month) - 1);
    time.setUTCDate(parseInt(day));
    const FULL_DAY = 24*60*60*1000; // 24*60*60*1000
    const toTime = new Date(+time + FULL_DAY);
    const satLog = {};
    const satTable = [];

    continuosTick(time, toTime, deltaTime, from, satLog, satTable).then(_ => {

        // sort satTable
        satTable.sort((a, b) => {
            if(a.starting != b.starting) {
                return a.starting > b.starting ? 1 : -1;
            }
            if(a.satName == b.satName) {
                return a.transit > b.transit ? 1 : -1;
            }
            return a.satName > b.satName ? 1 : -1;
        });
        // summary table
        satTable.forEach(satItem => {
            const total = satItem.detailed.sunlit.length
            const sunlit = satItem.detailed.sunlit.filter(sunlit => sunlit).length
            satItem.sunlitRatio = sunlit / total
        });
        postMessage({
            type: 'predictionFinished',
            satTable
        });
        timeMode = 'PREDICTION_RESULTS';

    });
    
}

function continuosTick(time, toTime, deltaTime, from, satLog, satTable) {
    return new Promise((resolve, reject) => {
        function continuosTick_internal() {
            tick(time, from, satLog, satTable);
            time = new Date(+time + deltaTime);
            if(time <= toTime) {
                setTimeout(_ => continuosTick_internal(), 0);
            } else {
                resolve(0);
            }
        }
        continuosTick_internal();
    });
}

function requestDataFrame() {

    if(timeMode === 'PLAY') {
        time = new Date(+time + 1000);
        sleepMs = 1000;
    } else if(timeMode === 'STOP') {
        time = new Date()
    } else if(timeMode === 'FORWARD') {
        time = new Date(+time + 20000);
        sleepMs = 0;
    } else if(timeMode === 'REWIND') {
        time = new Date(+time - 20000);
        sleepMs = 0;
    }

    tick(time);
    if(timeMode !== 'STOP' && timeMode !== 'PREDICTION') {
        setTimeout(requestDataFrame, sleepMs);
    }

}

function tick(time, from, satLog, satTable) {
    const gmst = satellite.gstime(time);

    const dataArraySize = satData.length * 4 + 6
    const dataArray = new Float32Array(dataArraySize);
    let cursor = 0;

    const UTCFullYear = time.getUTCFullYear();
    const UTCMonth = time.getUTCMonth();
    const UTCDate = time.getUTCDate();
    const UTCHours = time.getUTCHours();
    const UTCMinutes = time.getUTCMinutes();
    const UTCSeconds = time.getUTCSeconds();
    dataArray[cursor++] = UTCFullYear;
    dataArray[cursor++] = UTCMonth;
    dataArray[cursor++] = UTCDate;
    dataArray[cursor++] = UTCHours;
    dataArray[cursor++] = UTCMinutes;
    dataArray[cursor++] = UTCSeconds;

    const solarVector = calculateSolarPosition(time);

    satData.forEach((d) => {
        const eci = SGP4.predict(d.satrec, UTCFullYear, UTCMonth + 1, UTCDate, UTCHours, UTCMinutes, UTCSeconds);//satellite.sgp4(d.satrec, m);
        
        if (eci.pos) {
            const gdPos = satellite.eciToGeodetic(eci.pos, gmst);
            d.lat = satellite.radiansToDegrees(gdPos.latitude);
            d.lng = satellite.radiansToDegrees(gdPos.longitude);
            d.alt = gdPos.height / EARTH_RADIUS_KM;

            //let eci_old = satellite.sgp4(d.satrec, m);
            //console.log('eci', eci, eci_old);

            if(from) {
                const positionEcf = satellite.eciToEcf(eci.pos, gmst);
                const lookAngles = satellite.ecfToLookAngles(from, positionEcf);
                if(lookAngles.elevation * (180/Math.PI) >= from.minElevation) {
                    if(!(d.name in satLog)) {
                        satLog[d.name] = {
                            transits: [{
                                time: [], 
                                azimuth: [], 
                                elevation: [], 
                                rangeSat: [],
                                sunlit: [],
                                eclipseDepth: []
                            }],
                            _transitIndex: 0,
                            _prevElevation: 0,
                            _isRising: true
                        }
                    }
                    // calculate if eclipsed
                    const eclipse = satEclipsed(eci.pos, solarVector);

                    let satItem = satLog[d.name];
                    const isRisingNow = lookAngles.elevation >= satItem._prevElevation;
                    satItem._prevElevation = lookAngles.elevation;
                    if(!satItem._isRising && isRisingNow) {
                        // in this case we send transit and create a new one.
                        const maxElevation = satItem.transits[satItem._transitIndex].elevation.reduce((a, b) => Math.max(a, b));
                        const indexMaxElevation = satItem.transits[satItem._transitIndex].elevation.indexOf(maxElevation);
                        /*const dateStarting = satItem.transits[satItem._transitIndex].time[0];
                        const dateEnding = satItem.transits[satItem._transitIndex].time[satItem.transits[satItem._transitIndex].time.length - 1];
                        const dateStartingStr = */
                        satTable.push({
                            id: `${d.name}:${satItem._transitIndex}`,
                            satName: d.name,
                            transit: satItem._transitIndex + 1,
                            starting: satItem.transits[satItem._transitIndex].time[0],
                            ending: satItem.transits[satItem._transitIndex].time[satItem.transits[satItem._transitIndex].time.length - 1],
                            maxElevation: ra2dms(maxElevation),
                            apexAximuth: ra2dms(satItem.transits[satItem._transitIndex].azimuth[indexMaxElevation]),
                            detailed: satItem.transits[satItem._transitIndex],
                            sunlitRatio: null // this will be calculated later
                        });
                        satItem._transitIndex++;
                        satItem.transits.push({
                            time: [], 
                            azimuth: [], 
                            elevation: [], 
                            rangeSat: [],
                            sunlit: [],
                            eclipseDepth: []
                        });
                    }
                    const transit = satItem.transits[satItem._transitIndex];
                    satItem._isRising = isRisingNow;
                    transit.time.push(time);
                    transit.azimuth.push(lookAngles.azimuth);
                    transit.elevation.push(lookAngles.elevation);
                    transit.rangeSat.push(lookAngles.rangeSat);
                    transit.sunlit.push(!eclipse.eclipsed);
                    transit.eclipseDepth.push(eclipse.depth / deg2rad);
                    if(d.colorIndex != 11) {
                        d.colorIndex = eclipse.eclipsed ? 10 : 11 /* 10: eclipsed, 11: sinlit */
                    }
                }
            }

            dataArray[cursor++] = d.colorIndex
            dataArray[cursor++] = d.lat
            dataArray[cursor++] = d.lng
            dataArray[cursor++] = d.alt
        } else {
            dataArray[cursor++] = -1 // error no position
            dataArray[cursor++] = 0
            dataArray[cursor++] = 0
            dataArray[cursor++] = 0
        }
    });

    postMessage(dataArray.buffer, [dataArray.buffer]);
}

const pad2 = time => String(time).padStart(2, '0');

function ra2dms(ra) {
    const deg = ra * RAD_TO_DEG_FACTOR;
    const d = Math.floor(deg),
       rest1 = (deg - d) * 60,
       m = Math.floor(rest1),
       rest2 = (rest1 - m) * 60,
       s = Math.round(rest2);
    return `${pad2(d)}° ${pad2(m)}′ ${pad2(s)}″`;
}

function date2time(date) {
    return `${pad2(date.getUTCHours())}:${pad2(date.getUTCMinutes())}:${pad2(date.getUTCSeconds())}`
}



// ---------------------------------------------------------------------------------------------------------------------------
// The code bellow this sector is a modified version of https://github.com/nsat/jspredict/

const ms2day = 1000 * 60 * 60 * 24; // milliseconds to day
const deg2rad = Math.PI / 180;
const xkmper = 6.378137E3; // earth radius (km) wgs84
const astro_unit = 1.49597870691E8; // Astronomical unit - km (IAU 76)
const solar_radius = 6.96000E5; // solar radius - km (IAU 76)

/**
 * @author https://github.com/nsat/jspredict/
 * @param {any} start 
 * @returns 
 */
const calculateSolarPosition = start => {
    var time = start / ms2day + 2444238.5; // jul_utc

    var mjd = time - 2415020.0;
    var year = 1900 + mjd / 365.25;
    var T = (mjd + deltaET(year) / (ms2day / 1000)) / 36525.0;
    var M = deg2rad * ((358.47583 + ((35999.04975 * T) % 360) - (0.000150 + 0.0000033 * T) * Math.pow(T, 2)) % 360);
    var L = deg2rad * ((279.69668 + ((36000.76892 * T) % 360) + 0.0003025 * Math.pow(T, 2)) % 360);
    var e = 0.01675104 - (0.0000418 + 0.000000126 * T) * T;
    var C = deg2rad * ((1.919460 - (0.004789 + 0.000014 * T) * T) * Math.sin(M) + (0.020094 - 0.000100 * T) * Math.sin(2 * M) + 0.000293 * Math.sin(3 * M));
    var O = deg2rad * ((259.18 - 1934.142 * T) % 360.0);
    var Lsa = (L + C - deg2rad * (0.00569 - 0.00479 * Math.sin(O))) % (2 * Math.PI);
    var nu = (M + C) % (2 * Math.PI);
    var R = 1.0000002 * (1 - Math.pow(e, 2)) / (1 + e * Math.cos(nu));
    var eps = deg2rad * (23.452294 - (0.0130125 + (0.00000164 - 0.000000503 * T) * T) * T + 0.00256 * Math.cos(O));
    var R = astro_unit * R;

    return {
      x: R * Math.cos(Lsa),
      y: R * Math.sin(Lsa) * Math.cos(eps),
      z: R * Math.sin(Lsa) * Math.sin(eps),
      w: R
    }
}

/**
 * 
 * @param {*} pos 
 * @param {*} sol 
 * @returns 
 */
const satEclipsed = (pos, sol) => {
    var sd_earth = Math.asin(xkmper / magnitude(pos));
    var rho = vecSub(sol, pos);
    var sd_sun = Math.asin(solar_radius / rho.w);
    var earth = scalarMultiply(-1, pos);
    var delta = angle(sol, earth);

    var eclipseDepth = sd_earth - sd_sun - delta;
    var eclipse;
    if (sd_earth < sd_sun) {
      eclipse = false;
    } else if (eclipseDepth >= 0) {
      eclipse = true;
    } else {
      eclipse = false;
    }
    return {
      depth: eclipseDepth,
      eclipsed: eclipse
    }
}

/**
 * 
 * @param {number} year 
 * @returns 
 */
 const deltaET = year => {
    return 26.465 + 0.747622 * (year - 1950) + 1.886913 * Math.sin((2 * Math.PI) * (year - 1975) / 33)
}

/**
 * 
 * @param {{x:number, y:number, z:number}} v 
 * @returns 
 */
const magnitude = function(v) {
    return Math.sqrt(Math.pow(v.x, 2) + Math.pow(v.y, 2) + Math.pow(v.z, 2))
}

/**
 * 
 * @param {*} v1 
 * @param {*} v2 
 * @returns 
 */
const vecSub = (v1, v2) => {
    let vec = {
      x: v1.x - v2.x,
      y: v1.y - v2.y,
      z: v1.z - v2.z,
      w: 0,
    }
    vec.w = magnitude(vec);
    return vec
}

/**
 * 
 * @param {*} k 
 * @param {*} v 
 * @returns 
 */
const scalarMultiply = (k, v) => {
    return {
      x: k * v.x,
      y: k * v.y,
      z: k * v.z,
      w: v.w ? Math.abs(k) * v.w : undefined
    }
}

/**
 * 
 * @param {*} v1 
 * @param {*} v2 
 * @returns 
 */
const angle = (v1, v2) => {
    var dot = (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
    return Math.acos(dot / (magnitude(v1) * magnitude(v2)))
}