// Optional independent comparison against a locally supplied vendor bundle.
// Usage: ./check.sh && node tests/verify_vendor.cjs /path/to/main.e2372c083cc6b94f.js
// The bundle is inspected in a VM with no browser, network or HID capabilities.
const fs = require('fs'), vm = require('vm'), crypto = require('crypto'), cp = require('child_process'), path = require('path');
const source = fs.readFileSync(process.argv[2], 'utf8');
if (crypto.createHash('sha256').update(source).digest('hex') !== '1916df59569691fef1981e3b68c53186857804ba7e85c44bcc72251bee2478a4') throw Error('Unexpected vendor bundle');
const ctx = {self: {webpackChunk: []}};
vm.createContext(ctx);
vm.runInContext(source, ctx, {timeout: 5000});
const modules = ctx.self.webpackChunk[0][1], cache = {};
function req(id) {
    if (cache[id]) return cache[id].exports;
    const module = cache[id] = {exports: {}};
    modules[id](module, module.exports, req);
    return module.exports;
}
req.d = (obj, defs) => { for (const [key, get] of Object.entries(defs)) Object.defineProperty(obj, key, {get, enumerable: true}); };
req.r = obj => Object.defineProperty(obj, '__esModule', {value: true});
req.n = m => { const get = m && m.__esModule ? () => m.default : () => m; req.d(get, {a: get}); return get; };
const fx = req(7931);
const start = source.indexOf('const $n='), end = source.indexOf('var Or=', start);
vm.runInContext('var tn={shift:0,still:1,breathe:2,meshfify:3,fadeIn:4,instant:5,lavaLamp:6,waveRipple:7,twoColorFade:8,radius:9,off:10,torrent:11},Un=100,a={Em:{FRACTAL_SETTINGS:0}};' + source.slice(start, end) + ';globalThis.themes=Pr;', ctx);
function reference(theme, leds, brightness, speed, orientation, wave) {
    vm.runInContext(`Math.random=(()=>{let s=${1 + (theme.id << 8) + leds};return ()=>{s^=s<<13;s^=s>>>17;s^=s<<5;return (s>>>0)/4294967296}})()`, ctx);
    const colors = theme.colors.map(c => [1,3,5].map(i => parseInt(c.slice(i, i + 2), 16)));
    const args = [leds, speed, brightness], mirror = orientation >= 8, rotation = orientation % 8;
    const effect = theme.mode === 0 ? new fx.Pb(...args, colors, mirror, rotation) :
        theme.mode === 6 ? new fx.Cv(...args, colors, mirror, rotation) :
        theme.mode === 7 ? new fx.Lc(...args, ...colors, ...wave, mirror, rotation) :
        theme.mode === 8 ? new fx.Cj(...args, ...colors, mirror, rotation) :
        theme.mode === 1 ? new fx.ve(leds, brightness, colors[0], mirror, rotation) :
        new fx.Ku(...args, ...colors, mirror, rotation);
    const program = Buffer.from(effect.calcTimestamps()), header = Buffer.alloc(64);
    header.set([2,164,17,1,program.reduce((sum, x) => (sum + x) & 255, 0),program.length >> 8, program.length & 255,1,1]);
    header.set([theme.id, ...effect.getMetadata()], 9);
    return [header, program];
}
let count = 0;
function check(mode, theme, leds, brightness, speed, orientation, wave = [6,3,6,100]) {
    const packed = (wave[0] | wave[1] << 8 | wave[2] << 16 | wave[3] << 24) >>> 0;
    const expected = reference(theme, leds, brightness, speed, orientation, wave);
    const actual = cp.execFileSync(path.join(__dirname, '../build/test_protocol'), ['--dump',mode,leds,brightness,speed,orientation,packed,theme.colors.map(c => c.slice(1)).join('')].map(String), {encoding:'utf8'}).trim().split('\n');
    if (actual.some((hex, i) => hex !== expected[i].toString('hex'))) throw Error(`Mismatch: ${JSON.stringify({mode,leds,brightness,speed,orientation,wave})}`);
    count++;
}
for (let i = 0; i < ctx.themes.length; i++)
    for (const leds of [3,11,20,35,76,255])
    for (const brightness of [0,25,100])
    for (const speed of [0,ctx.themes[i].speed,100])
    for (const orientation of [0,11]) check(i + 5, ctx.themes[i], leds, brightness, speed, orientation);
const custom = [
    [15,0,['#1322C9','#FC7F03','#010203','#808182','#C8C9CA','#FF0063']],
    [16,7,['#1322C9','#FC7F03']], [17,8,['#1322C9','#FC7F03']],
    [18,6,['#1322C9','#FC7F03','#010203','#808182','#C8C9CA','#FF0063']],
    [1,1,['#1322C9']], [3,2,['#1322C9','#FC7F03']]
];
for (const [mode,kind,colors] of custom)
    for (const leds of [3,11,20,35,76,255])
    for (const brightness of [0,25,100])
    for (const speed of [0,73,100])
    for (const orientation of [0,11]) check(mode, {id:153,mode:kind,colors}, leds, brightness, speed, orientation, [25,0,7,31]);
for (const kind of [3,4,5,10]) for (const leds of [3,11,20,35,76,255]) for (const brightness of [0,25,100]) {
    const color = kind === 10 ? [0,0,0] : [19,34,201];
    const effect = kind === 3 ? new fx.cQ(leds,brightness,color,false,0) : kind === 4 ? new fx.YH(leds,brightness,color,false,0) : kind === 5 ? new fx.WU(leds,brightness,color,false,0) : new fx.DZ(leds,brightness,color,false,0);
    const p = Buffer.from(effect.calcTimestamps()), h = Buffer.alloc(64);
    h.set([2,164,17,0,p.reduce((sum,x)=>(sum+x)&255,0),p.length>>8,p.length&255,0,1]);
    h.set([153,...effect.getMetadata()],9);
    const out = cp.execFileSync(path.join(__dirname,'../build/test_protocol'), ['--startup-dump',kind,leds,brightness,Buffer.from(color).toString('hex')].map(String), {encoding:'utf8'}).split('\n');
    if(out[0] !== h.toString('hex') || out[1] !== p.toString('hex')) throw Error('Startup mismatch: '+JSON.stringify({kind,leds,brightness}));
    count++;
}
console.log(`PASS: ${count} complete headers/programs match the vendor encoder`);
