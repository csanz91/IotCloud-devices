#pragma once

#include <Arduino.h>

// Self-contained page: no CDN, no external fonts, no internet needed.
static const char INDEX_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Headlight Lux</title>
<style>
:root{--bg:#0e1116;--card:#171b22;--line:#2a303b;--fg:#e6e9ef;--mut:#8b93a1;--acc:#ffb340;--ok:#3fb950;--bad:#f85149;--warn:#d29922}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:1000px;margin:0 auto;padding:12px}
header{display:flex;align-items:center;gap:6px;flex-wrap:wrap;margin-bottom:10px}
h1{font-size:18px;margin:0;flex:1 1 auto}
.pill{font-size:12px;padding:2px 9px;border-radius:99px;background:var(--line);color:var(--mut);white-space:nowrap}
.pill.ok{background:#12361d;color:var(--ok)}.pill.bad{background:#3d1416;color:var(--bad)}
.pill.rec{background:#3d1416;color:#ff7b72}.pill.arm{background:#3a2c0b;color:var(--warn)}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px;margin-bottom:12px}
.big{font-size:56px;font-weight:700;font-variant-numeric:tabular-nums;line-height:1.1;color:var(--acc)}
.big small{font-size:20px;color:var(--mut);font-weight:400}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:flex-end}
.between{justify-content:space-between;align-items:center}
.kv{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:6px 12px;margin-top:8px}
.kv div{font-size:12px;color:var(--mut)}.kv b{display:block;color:var(--fg);font-size:16px;font-weight:600;font-variant-numeric:tabular-nums}
label{font-size:12px;color:var(--mut);display:flex;flex-direction:column;gap:3px}
label.inl{flex-direction:row;align-items:center;gap:5px;font-size:13px}
input,select,button{font:inherit;color:var(--fg);background:var(--bg);border:1px solid var(--line);border-radius:7px;padding:7px 9px}
input[type=checkbox],input[type=radio]{padding:0;width:16px;height:16px;accent-color:var(--acc)}
button{background:#232a35;cursor:pointer}
button.pri{background:var(--acc);color:#111;border-color:var(--acc);font-weight:600}
button.danger{background:#5a1d1d;border-color:#7d2b2b}
button.sm{padding:3px 8px;font-size:12px}
button:disabled{opacity:.4;cursor:default}
a{color:#58a6ff}
canvas{width:100%;height:340px;display:block;touch-action:pan-y}
table{width:100%;border-collapse:collapse;font-size:13px;font-variant-numeric:tabular-nums}
th,td{padding:6px 5px;border-bottom:1px solid var(--line);text-align:right;white-space:nowrap}
th{color:var(--mut);font-weight:500}
td.l,th.l{text-align:left}
.scroll{overflow-x:auto}
.sw{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px;vertical-align:middle}
.msg{color:var(--bad);font-size:13px;min-height:1.2em;margin-top:6px}
.hint{color:var(--mut);font-size:12px}
.good{color:var(--ok)}.warn{color:var(--warn)}
#legend span{margin-right:12px;display:inline-block}
</style></head><body><main>
<header><h1>Headlight Lux Logger</h1><span id="pSensor" class="pill">sensor</span><span id="pState" class="pill">offline</span></header>

<section class="card">
 <div class="big"><span id="vNet">--</span> <small>lx net</small></div>
 <div class="kv">
  <div>Raw reading<b id="vRaw">--</b></div>
  <div>Ambient (subtracted)<b id="vAmb">--</b></div>
  <div>Sensor range<b id="vRange">--</b></div>
  <div>Trend, last 2 min<b id="vTrend">--</b></div>
 </div>
</section>

<section class="card">
 <div class="row between"><b>Ambient baseline</b>
  <div class="row"><button id="bAmb">Measure (lights off, 3 s)</button><button id="bAmbClr">Clear</button></div></div>
 <div class="hint" style="margin-top:6px">Headlights off, engine/ignition as during the run. The value is subtracted from every new run's samples (stored per run).</div>
</section>

<section class="card">
 <div class="row between"><b>Run</b><span id="vRun" class="hint"></span></div>
 <div class="row" style="margin-top:8px">
  <label style="flex:2 1 200px">Label<input id="iLabel" maxlength="40" placeholder="e.g. Left low beam, old bulb"></label>
  <label>Sample every<select id="iInt"><option value="1000">1 s</option><option value="2000">2 s</option><option value="5000">5 s</option><option value="10000">10 s</option></select></label>
  <label>Auto-stop<select id="iStop"><option value="0">off</option><option value="5">5 min</option><option value="10">10 min</option><option value="15">15 min</option><option value="20">20 min</option><option value="30">30 min</option><option value="40">40 min</option><option value="45">45 min</option><option value="60">60 min</option><option value="90">90 min</option></select></label>
  <label>Start when net lx &ge; <input id="iTrig" type="number" min="0" step="any" value="0" style="width:110px"></label>
 </div>
 <div class="hint" style="margin-top:6px">Start trigger 0 = start immediately. Set it (e.g. 5 lx) to arm the logger and start at t=0 the moment you switch the lights on, so runs line up.</div>
 <div class="row" style="margin-top:10px"><button id="bStart" class="pri">Start</button><button id="bStop" class="danger" disabled>Stop</button></div>
 <div id="msg" class="msg"></div>
</section>

<section class="card">
 <div class="row between"><b>Chart</b>
  <div class="row" style="align-items:center">
   <label class="inl"><input type="checkbox" id="cAlign" checked>Align at lamp-on</label>
   <label class="inl"><input type="checkbox" id="cNorm">% of plateau</label>
   <label class="inl"><input type="checkbox" id="cZero" checked>Y from 0</label>
   <label class="inl">Plateau = mean of last<select id="iWin"><option value="60">1 min</option><option value="120">2 min</option><option value="300">5 min</option><option value="600">10 min</option></select></label>
  </div></div>
 <canvas id="cv"></canvas>
 <div id="legend" class="hint"></div>
</section>

<section class="card">
 <b>Comparison</b> <span class="hint">(ticked runs + live run; Δ is relative to the Ref run)</span>
 <div class="scroll"><table id="tCmp"><thead><tr><th class="l">Ref</th><th class="l">Run</th><th>Plateau</th><th>Δ ref</th><th>Lamp on</th><th>Initial</th><th>Init&rarr;plat</th><th>Max</th><th>Settled &plusmn;5%</th><th>Settled &plusmn;2%</th><th>Trend</th><th>Noise</th><th>Range</th></tr></thead><tbody></tbody></table></div>
 <div class="hint" style="margin-top:6px">Lamp on = first sample above 15% of plateau; Initial (first 5 s after it) and Settled (time after which the curve stays within the band around the plateau) count from there. Range = sensor range(s) used in the plateau window; compare runs measured in the same range. Trend = slope over the plateau window. Noise = std. dev. in the plateau window.</div>
</section>

<section class="card">
 <div class="row between"><b>Saved runs</b><span id="vFs" class="hint"></span></div>
 <div class="scroll"><table id="tRuns"><thead><tr><th class="l"></th><th class="l">#</th><th class="l">Label</th><th>Date</th><th>Duration</th><th>Plateau 2m</th><th>Max</th><th></th></tr></thead><tbody></tbody></table></div>
 <div class="hint" style="margin-top:6px">Tick runs to overlay them. Values are net lux (ambient subtracted). CSV columns: elapsed_s (start of each sample window), lux_net, lux_raw, range (0 fine, 1 normal, 2 bright).</div>
</section>
</main>
<script>
"use strict";
const $=id=>document.getElementById(id);
const COLORS=['#ffb340','#58a6ff','#3fb950','#f778ba','#a371f7','#ff7b72','#39c5cf','#d29922','#8ddb8c','#e0823d'];
let st=null, prevState=null;
let live={id:0,stride:0,interval:1000,d:[]};
let runs=[];
const cache=new Map();
let sel=[];
let refId=null;
let hoverT=null, geo=null;

const LS={get(k,d){try{const v=localStorage.getItem('hl_'+k);return v===null?d:v}catch(e){return d}},
          set(k,v){try{localStorage.setItem('hl_'+k,v)}catch(e){}}};

async function api(path,params){
  const opt=params?{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(params)}:{cache:'no-store'};
  const r=await fetch(path,opt);
  const txt=await r.text();
  let j;try{j=JSON.parse(txt)}catch(e){j={error:txt}}
  if(!r.ok)throw new Error(j.error||('HTTP '+r.status));
  return j;
}
const isNum=v=>typeof v==='number'&&isFinite(v);
function fmt(v,d){if(!isNum(v))return '--';if(d===undefined)d=Math.abs(v)<10?2:Math.abs(v)<1000?1:0;return v.toFixed(d)}
function fmtPct(v,d){if(!isNum(v))return '--';return (v>0?'+':'')+v.toFixed(d===undefined?1:d)+'%'}
function fmtDur(s){if(!isNum(s))return '--';s=Math.round(s);const h=Math.floor(s/3600),m=Math.floor(s%3600/60),x=s%60;
  return (h?h+':'+String(m).padStart(2,'0'):m)+':'+String(x).padStart(2,'0')}
function fmtDate(e){if(!e)return '--';const d=new Date(e*1000);
  return d.toLocaleDateString(undefined,{month:'short',day:'numeric'})+' '+d.toLocaleTimeString(undefined,{hour:'2-digit',minute:'2-digit'})}
function msg(t){$('msg').textContent=t||''}
const colorFor=id=>COLORS[id%COLORS.length];
const RANGE_NAMES=['fine','normal','bright'];

// ---------- statistics ----------
function stats(t,y,win,rg){
  const n=y.length;if(!n)return null;
  const tEnd=t[n-1];
  let s=0,c=0,mx=-Infinity;const ranges=new Set();
  for(let i=0;i<n;i++){const v=y[i];if(!isNum(v))continue;
    if(v>mx)mx=v;
    if(t[i]>tEnd-win){s+=v;c++;if(rg&&rg[i]!==null)ranges.add(rg[i])}}
  if(!c)return null;
  const p=s/c;
  // Lamp-on = first sample above 15% of plateau. If darker samples come
  // before it (run started by hand, lamp switched on later), that sample was
  // only partly lit and is left out of "initial"; times are relative to it.
  let iOn=0;
  if(p>0){iOn=y.findIndex(v=>isNum(v)&&v>=0.15*p);if(iOn<0)iOn=0}
  const tOn=t[iOn],i0=iOn>0?iOn+1:iOn;
  let si=0,ci=0;
  for(let i=i0;i<n&&t[i]<t[i0]+5;i++){const v=y[i];if(isNum(v)){si+=v;ci++}}
  let ss=0,sx=0,sy=0,sxx=0,sxy=0;
  for(let i=0;i<n;i++){const v=y[i];if(!isNum(v)||t[i]<=tEnd-win)continue;
    ss+=(v-p)*(v-p);const x=t[i]/60;sx+=x;sy+=v;sxx+=x*x;sxy+=x*v}
  const sd=c>1?Math.sqrt(ss/(c-1)):0;
  const den=c*sxx-sx*sx;
  const slope=c>2&&den>0?(c*sxy-sx*sy)/den:NaN;  // lux per minute
  const settle=f=>{if(!(p>0))return NaN;
    for(let i=n-1;i>=iOn;i--){const v=y[i];if(isNum(v)&&Math.abs(v-p)>f*p)return i+1<n?t[i+1]-tOn:NaN}
    return 0};
  const init=ci?si/ci:NaN;
  return {plateau:p,max:mx,initial:init,initPct:init>0?(p/init-1)*100:NaN,noisePct:p>0?sd/p*100:NaN,
          slopePct:p>0?slope/p*100:NaN,s5:settle(0.05),s2:settle(0.02),dur:tEnd,tOn,ranges:[...ranges].sort()};
}

// ---------- series ----------
function liveSeries(){
  if(!st||st.state!=='rec'||!live.d.length||live.id!==st.run.id)return null;
  const k=live.stride*live.interval/1000;
  return {id:'live',label:'LIVE #'+live.id+' '+st.run.label,t:live.d.map((_,i)=>i*k),y:live.d,color:'#ffffff',live:true};
}
function seriesList(){
  const out=[];
  for(const id of sel){const c=cache.get(id);if(!c)continue;const m=runs.find(r=>r.id===id);
    out.push({id,label:'#'+id+' '+(m?m.label:''),t:c.t,y:c.y,r:c.r,color:colorFor(id)})}
  const l=liveSeries();if(l)out.push(l);
  return out;
}

// ---------- chart ----------
function ticks(a,b,n){const span=b-a;if(!(span>0))return [a];
  const raw=span/n,p=Math.pow(10,Math.floor(Math.log10(raw))),m=raw/p;
  const step=(m<1.5?1:m<3?2:m<7?5:10)*p;const out=[];
  for(let v=Math.ceil(a/step-1e-9)*step;v<=b+step*1e-9;v+=step)out.push(+v.toFixed(10));
  return out}
function nearest(t,x){let lo=0,hi=t.length-1;if(hi<0)return -1;
  while(hi-lo>1){const mid=(lo+hi)>>1;if(t[mid]<x)lo=mid;else hi=mid}
  return Math.abs(t[lo]-x)<=Math.abs(t[hi]-x)?lo:hi}

function draw(){
  const cv=$('cv'),dpr=window.devicePixelRatio||1,W=cv.clientWidth,H=cv.clientHeight;
  if(cv.width!==Math.round(W*dpr)||cv.height!==Math.round(H*dpr)){cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr)}
  const g=cv.getContext('2d');g.setTransform(dpr,0,0,dpr,0,0);g.clearRect(0,0,W,H);
  const norm=$('cNorm').checked,win=+$('iWin').value,zero=$('cZero').checked,align=$('cAlign').checked;
  const S=seriesList().map(s=>{const q=stats(s.t,s.y,win,s.r);let yy=s.y,t=s.t;
    if(norm){const p=q&&q.plateau;yy=s.y.map(v=>isNum(v)&&p>0?v/p*100:null)}
    if(align&&q&&q.tOn>0){const o=q.tOn;t=s.t.map(x=>x-o);yy=yy.map((v,i)=>t[i]<0?null:v)}
    return Object.assign({},s,{t,yy})});
  let tMax=60,yMin=Infinity,yMax=-Infinity;
  for(const s of S){if(s.t.length)tMax=Math.max(tMax,s.t[s.t.length-1]);
    for(const v of s.yy)if(isNum(v)){if(v<yMin)yMin=v;if(v>yMax)yMax=v}}
  if(!isFinite(yMin)){yMin=0;yMax=norm?110:10}
  if(zero)yMin=Math.min(0,yMin);
  if(yMax-yMin<1e-6){yMax+=1;yMin-=zero&&yMin===0?0:1}
  const pad=(yMax-yMin)*0.06;yMax+=pad;if(!(zero&&yMin===0))yMin-=pad;
  const L=54,R=12,T=10,B=30,pw=W-L-R,ph=H-T-B;
  const tm=tMax/60;
  const X=t=>L+(t/60)/tm*pw,Y=v=>T+(1-(v-yMin)/(yMax-yMin))*ph;
  geo={L,pw,tMax};
  g.font='11px system-ui,sans-serif';g.lineWidth=1;
  // grid + labels
  g.strokeStyle='#2a303b';g.fillStyle='#8b93a1';g.textAlign='right';g.textBaseline='middle';
  for(const v of ticks(yMin,yMax,6)){const y=Y(v);g.beginPath();g.moveTo(L,y);g.lineTo(L+pw,y);g.stroke();
    g.fillText(norm?v.toFixed(v%1?1:0)+'%':fmt(v),L-5,y)}
  g.textAlign='center';g.textBaseline='top';
  for(const v of ticks(0,tm,Math.max(3,Math.floor(pw/70)))){const x=L+v/tm*pw;g.beginPath();g.moveTo(x,T);g.lineTo(x,T+ph);g.stroke();
    g.fillText(v+(v===0?' min':''),x,T+ph+6)}
  if(norm){g.strokeStyle='#555d6b';g.setLineDash([4,4]);g.beginPath();g.moveTo(L,Y(100));g.lineTo(L+pw,Y(100));g.stroke();g.setLineDash([])}
  // series
  g.save();g.beginPath();g.rect(L,T,pw,ph);g.clip();
  for(const s of S){g.strokeStyle=s.color;g.lineWidth=s.live?2:1.6;g.beginPath();let pen=false;
    for(let i=0;i<s.yy.length;i++){const v=s.yy[i];if(!isNum(v)){pen=false;continue}
      const x=X(s.t[i]),y=Y(v);if(pen)g.lineTo(x,y);else g.moveTo(x,y);pen=true}
    g.stroke()}
  g.restore();
  // hover readout
  if(hoverT!==null&&S.length){const x=X(hoverT);g.strokeStyle='#8b93a1';g.beginPath();g.moveTo(x,T);g.lineTo(x,T+ph);g.stroke();
    const lines=[fmtDur(hoverT)];const cols=['#e6e9ef'];
    for(const s of S){const i=nearest(s.t,hoverT);if(i<0)continue;const v=s.yy[i];
      lines.push(s.label.slice(0,22)+': '+(isNum(v)?(norm?v.toFixed(1)+'%':fmt(v)+' lx'):'--'));cols.push(s.color);
      if(isNum(v)){g.fillStyle=s.color;g.beginPath();g.arc(X(s.t[i]),Y(v),3,0,7);g.fill()}}
    g.font='12px system-ui,sans-serif';g.textAlign='left';g.textBaseline='top';
    const w=Math.max(...lines.map(l=>g.measureText(l).width))+12,h=lines.length*16+8;
    let bx=x+10;if(bx+w>L+pw)bx=x-10-w;if(bx<L)bx=L;
    g.fillStyle='rgba(14,17,22,.92)';g.fillRect(bx,T+4,w,h);g.strokeStyle='#2a303b';g.strokeRect(bx,T+4,w,h);
    lines.forEach((l,i)=>{g.fillStyle=cols[i];g.fillText(l,bx+6,T+8+i*16)})}
  // legend
  $('legend').innerHTML='';
  for(const s of S){const sp=document.createElement('span');const sw=document.createElement('i');sw.className='sw';sw.style.background=s.color;
    sp.appendChild(sw);sp.appendChild(document.createTextNode(s.label));$('legend').appendChild(sp)}
  if(!S.length)$('legend').textContent='Start a run or tick saved runs to see curves. Time axis in minutes from start.';
}
function onHover(e){if(!geo)return;const r=$('cv').getBoundingClientRect();const x=e.clientX-r.left;
  if(x<geo.L||x>geo.L+geo.pw){hoverT=null}else{hoverT=(x-geo.L)/geo.pw*geo.tMax}draw()}
$('cv').addEventListener('pointermove',onHover);
$('cv').addEventListener('pointerdown',onHover);
$('cv').addEventListener('pointerleave',()=>{hoverT=null;draw()});

// ---------- comparison ----------
function renderCmp(){
  const tb=$('tCmp').tBodies[0];tb.innerHTML='';
  const win=+$('iWin').value;
  const S=seriesList();
  if(refId===null||!S.some(s=>s.id===refId))refId=S.length?S[0].id:null;
  const rows=S.map(s=>({s,q:stats(s.t,s.y,win,s.r)}));
  const ref=rows.find(r=>r.s.id===refId);
  const pref=ref&&ref.q?ref.q.plateau:NaN;
  for(const {s,q} of rows){
    const tr=document.createElement('tr');
    const td=(v,cls)=>{const c=document.createElement('td');if(cls)c.className=cls;if(v instanceof Node)c.appendChild(v);else c.textContent=v;tr.appendChild(c);return c};
    const rb=document.createElement('input');rb.type='radio';rb.name='ref';rb.checked=s.id===refId;
    rb.onchange=()=>{refId=s.id;renderCmp()};td(rb,'l');
    const lab=document.createElement('span');const sw=document.createElement('i');sw.className='sw';sw.style.background=s.color;
    lab.appendChild(sw);lab.appendChild(document.createTextNode(s.label));td(lab,'l');
    if(!q){for(let i=0;i<11;i++)td('--');tb.appendChild(tr);continue}
    const short=q.dur<win*0.999;
    td(fmt(q.plateau)+(short?'*':''));
    td(s.id===refId?'ref':(pref>0?fmtPct((q.plateau/pref-1)*100):'--'));
    td(fmtDur(q.tOn));td(fmt(q.initial));td(fmtPct(q.initPct));td(fmt(q.max));
    td(isNum(q.s5)?fmtDur(q.s5):'not yet');td(isNum(q.s2)?fmtDur(q.s2):'not yet');
    const tc=td(isNum(q.slopePct)?fmtPct(q.slopePct,2)+'/min':'--');
    if(isNum(q.slopePct))tc.className=Math.abs(q.slopePct)<0.3?'good':'warn';
    td('±'+fmt(q.noisePct,2)+'%');
    const rc=td(q.ranges.length?q.ranges.map(i=>RANGE_NAMES[i]||i).join('+'):'--');
    if(q.ranges.length>1)rc.className='warn';
    tb.appendChild(tr)}
  if(!rows.length){tb.innerHTML='<tr><td class="l hint" colspan="13">Tick saved runs below (or start a run) to compare them.</td></tr>'}
}

// ---------- status / live ----------
async function pollStatus(){
  try{st=await api('/api/status');renderStatus()}
  catch(e){$('pState').textContent='offline';$('pState').className='pill bad'}
  setTimeout(pollStatus,1000);
}
function renderStatus(){
  const ps=$('pSensor');
  if(!st.ok){ps.textContent='SENSOR ERROR';ps.className='pill bad'}
  else if(st.sat){ps.textContent='SATURATED';ps.className='pill bad'}
  else{ps.textContent='sensor ok';ps.className='pill ok'}
  $('vNet').textContent=fmt(st.net);
  $('vRaw').textContent=isNum(st.lux)?fmt(st.lux)+' lx':'--';
  $('vAmb').textContent=st.ambBusy?'measuring…':fmt(st.amb)+' lx';
  $('vRange').textContent=st.range+' · '+st.res.toFixed(2)+' lx/ct';
  const pst=$('pState');const r=st.run;
  if(st.state==='rec'){pst.textContent='REC '+fmtDur(st.elapsed);pst.className='pill rec';
    $('vRun').textContent='#'+r.id+' '+r.label+' · '+r.n+' samples · auto-stop '+fmtDur(st.autostop)+' · ambient '+fmt(r.ambient)+' lx'}
  else if(st.state==='armed'){pst.textContent='ARMED';pst.className='pill arm';
    $('vRun').textContent='Waiting for net lux ≥ '+fmt(r.trigger)+' … switch the lights on'}
  else{pst.textContent='idle';pst.className='pill';$('vRun').textContent=''}
  $('bStart').disabled=st.state!=='idle';$('bStop').disabled=st.state==='idle';
  $('bStop').textContent=st.state==='armed'?'Cancel':'Stop';
  if(st.err)msg(st.err);
  $('vFs').textContent='flash '+Math.round(st.fsUsed/1024)+' / '+Math.round(st.fsTotal/1024)+' kB'+(st.clock?'':' · clock not set');
  const l=liveSeries();
  if(l){const q=stats(l.t,l.y,120);const el=$('vTrend');
    if(q&&isNum(q.slopePct)&&q.dur>=30){el.textContent=fmtPct(q.slopePct,2)+'/min '+(Math.abs(q.slopePct)<0.3?'stable':Math.sign(q.slopePct)>0?'rising':'falling');
      el.className=Math.abs(q.slopePct)<0.3?'good':'warn'}else{el.textContent='collecting…';el.className=''}}
  else{$('vTrend').textContent='--';$('vTrend').className=''}
  if(prevState!==null&&prevState!==st.state){
    if(st.state==='idle'&&prevState==='rec'){onRunFinished(st.lastId)}
    else loadRuns()}
  prevState=st.state;
}
async function onRunFinished(id){
  await loadRuns();
  if(id&&!sel.includes(id)&&runs.some(r=>r.id===id)){await toggleSel(id,true)}
  draw();renderCmp();
}
async function pollLive(){
  if(st&&st.state==='rec'){
    try{
      const j=await api('/api/live?id='+live.id+'&from='+live.d.length+'&stride='+live.stride);
      if(j.from===0||j.id!==live.id||j.stride!==live.stride)live.d=j.d;
      else live.d=live.d.slice(0,j.from).concat(j.d);
      live.id=j.id;live.stride=j.stride;live.interval=j.interval;
      draw();renderCmp();
    }catch(e){}
  }
  setTimeout(pollLive,2000);
}

// ---------- runs ----------
async function loadRuns(){
  try{runs=await api('/api/runs');runs.sort((a,b)=>b.id-a.id)}catch(e){return}
  sel=sel.filter(id=>runs.some(r=>r.id===id));
  renderRuns();
}
function renderRuns(){
  const tb=$('tRuns').tBodies[0];tb.innerHTML='';
  const active=st&&st.state==='rec'?st.run.id:-1;
  for(const r of runs){
    const tr=document.createElement('tr');
    const td=(v,cls)=>{const c=document.createElement('td');if(cls)c.className=cls;if(v instanceof Node)c.appendChild(v);else c.textContent=v;tr.appendChild(c);return c};
    const cb=document.createElement('input');cb.type='checkbox';cb.checked=sel.includes(r.id);cb.disabled=r.id===active;
    cb.onchange=()=>toggleSel(r.id,cb.checked);td(cb,'l');
    const sw=document.createElement('span');const i=document.createElement('i');i.className='sw';i.style.background=colorFor(r.id);
    sw.appendChild(i);sw.appendChild(document.createTextNode(r.id));td(sw,'l');
    td(r.label+(r.id===active?' (recording)':r.recovered?' (recovered)':''),'l');
    td(fmtDate(r.start));td(fmtDur(r.dur));td(fmt(r.plateau));td(fmt(r.max));
    const act=document.createElement('span');
    const a=document.createElement('a');a.href='/api/run.csv?id='+r.id;a.textContent='CSV';a.setAttribute('download','');act.appendChild(a);
    act.appendChild(document.createTextNode(' '));
    const del=document.createElement('button');del.className='sm';del.textContent='Delete';del.disabled=r.id===active;
    del.onclick=async()=>{if(!confirm('Delete run #'+r.id+' "'+r.label+'"? This cannot be undone.'))return;
      try{await api('/api/delete',{id:r.id});cache.delete(r.id);sel=sel.filter(x=>x!==r.id);await loadRuns();draw();renderCmp()}catch(e){msg(e.message)}};
    act.appendChild(del);td(act);
    tb.appendChild(tr)}
  if(!runs.length)tb.innerHTML='<tr><td class="l hint" colspan="8">No runs saved yet.</td></tr>';
}
async function ensureData(id){
  if(cache.has(id))return;
  const r=await fetch('/api/run.csv?id='+id,{cache:'no-store'});if(!r.ok)throw new Error('cannot load run '+id);
  const t=[],y=[],rg=[];
  for(const line of (await r.text()).split('\n')){const p=line.split(',');if(p.length<2)continue;
    const tv=parseFloat(p[0]);if(!isFinite(tv))continue;
    const yv=p[1]===''?null:parseFloat(p[1]);t.push(tv);y.push(isFinite(yv)?yv:null);
    const rv=p.length>3&&p[3]!==''?parseInt(p[3]):null;rg.push(Number.isInteger(rv)?rv:null)}
  cache.set(id,{t,y,r:rg});
}
async function toggleSel(id,on){
  if(on){try{await ensureData(id)}catch(e){msg(e.message);renderRuns();return}
    if(!sel.includes(id))sel.push(id);if(refId===null)refId=id}
  else{sel=sel.filter(x=>x!==id)}
  renderRuns();renderCmp();draw();
}

// ---------- controls ----------
function savePrefs(){for(const k of ['iLabel','iInt','iStop','iTrig','iWin'])LS.set(k,$(k).value);
  for(const k of ['cNorm','cZero','cAlign'])LS.set(k,$(k).checked?1:0)}
function loadPrefs(){for(const [k,d] of [['iLabel',''],['iInt','1000'],['iStop','40'],['iTrig','0'],['iWin','120']])$(k).value=LS.get(k,d);
  for(const [k,d] of [['cNorm','0'],['cZero','1'],['cAlign','1']])$(k).checked=LS.get(k,d)==='1'}
$('bStart').onclick=async()=>{
  savePrefs();msg('');
  try{await api('/api/start',{label:$('iLabel').value,interval:$('iInt').value,autostop:(+$('iStop').value)*60,trigger:$('iTrig').value||0});
    live={id:0,stride:0,interval:+$('iInt').value,d:[]}}
  catch(e){msg(e.message)}};
$('bStop').onclick=async()=>{try{await api('/api/stop',{})}catch(e){msg(e.message)}};
$('bAmb').onclick=async()=>{msg('');try{await api('/api/ambient',{action:'measure'})}catch(e){msg(e.message)}};
$('bAmbClr').onclick=async()=>{try{await api('/api/ambient',{action:'clear'})}catch(e){msg(e.message)}};
for(const id of ['cNorm','cZero','cAlign','iWin'])$(id).onchange=()=>{savePrefs();draw();renderCmp()};
for(const id of ['iLabel','iInt','iStop','iTrig'])$(id).onchange=savePrefs;
window.addEventListener('resize',draw);

loadPrefs();
api('/api/time',{ms:Date.now()}).catch(()=>{});
loadRuns().then(()=>{renderCmp();draw()});
pollStatus();
pollLive();
</script></body></html>
)rawhtml";
