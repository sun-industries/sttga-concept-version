const RAD_TO_DEG_FACTOR = 180 / Math.PI;

export const pad2 = time => String(time).padStart(2, '0');

export const ra2dms = ra => {
    const deg = ra * RAD_TO_DEG_FACTOR;
    const d = Math.floor(deg),
       rest1 = (deg - d) * 60,
       m = Math.floor(rest1),
       rest2 = (rest1 - m) * 60,
       s = Math.round(rest2);
    return `${pad2(d)}° ${pad2(m)}′ ${pad2(s)}″`;
}

export const date2time = date => {
    return `${pad2(date.getUTCHours())}:${pad2(date.getUTCMinutes())}:${pad2(date.getUTCSeconds())}`
}