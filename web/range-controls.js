// SPDX-License-Identifier: MIT OR GPL-3.0-only
// Focus persists after a native range gesture. Protect only an active gesture,
// so later device/worklet states can update a focused slider without stealing
// keyboard focus or dispatching input commands back to the instrument.
const rangeGestures=new Map();
const rangeKeys=new Set(['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Home','End','PageUp','PageDown']);
function rangeGesture(r){
 if(!rangeGestures.has(r))rangeGestures.set(r,{pointers:new Set(),keys:new Set()});
 return rangeGestures.get(r);
}
function releaseRange(kind,value){
 for(const [r,g] of rangeGestures){
  g[kind].delete(value);
  if(!g.pointers.size&&!g.keys.size)rangeGestures.delete(r);
 }
}
function bindRange(r){
 r.addEventListener('pointerdown',e=>rangeGesture(r).pointers.add(e.pointerId));
 r.addEventListener('lostpointercapture',e=>releaseRange('pointers',e.pointerId));
 r.addEventListener('keydown',e=>{if(rangeKeys.has(e.key))rangeGesture(r).keys.add(e.key);});
 r.addEventListener('blur',()=>rangeGestures.delete(r));
}
function syncRange(r,value){if(!rangeGestures.has(r))r.value=value;}
// Capture releases outside the input, too. Do not override native dragging,
// pointer capture, keyboard actions or accessibility focus.
window.addEventListener('pointerup',e=>releaseRange('pointers',e.pointerId),true);
window.addEventListener('pointercancel',e=>releaseRange('pointers',e.pointerId),true);
window.addEventListener('keyup',e=>releaseRange('keys',e.key),true);
window.addEventListener('blur',()=>rangeGestures.clear());
