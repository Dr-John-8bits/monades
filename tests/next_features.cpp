#include "dsp/dreams.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
static float audio[256];
static double run(int blocks){double e=0;for(int b=0;b<blocks;b++){md_render(audio,128);for(float x:audio){assert(std::isfinite(x)&&fabsf(x)<1);e+=x*x;}}return e;}
static void cmd(int c,int t,int v){assert(md_command(c,t,v));run(1);}
static md_patch blank(int model){md_init();cmd(MD_SCENE,0,64);md_patch p=*md_state();p.track[0].mute=0;p.track[0].model=model;p.track[0].note=36;p.track[0].level=650;p.track[0].decay=200;p.track[0].attack=0;p.track[0].hits=16;p.track[0].length=16;p.track[0].prob=1000;p.macro[0]=500;p.macro[1]=0;p.macro[2]=500;p.macro[3]=0;return p;}
static uint64_t hash(const md_patch &p){md_init();assert(md_load(&p));cmd(MD_PLAY,0,1);uint64_t h=1469598103934665603ull;for(int b=0;b<2200;b++){md_render(audio,128);for(float x:audio){uint32_t u;memcpy(&u,&x,4);h=(h^u)*1099511628211ull;}}return h;}
int main(){
 static_assert(sizeof(md_patch)==1496,"Browser patch size must match");
 auto p=blank(MD_BASS);assert(md_load(&p));cmd(MD_TRIGGER,0,36);assert(run(300)>1);puts("PASS round bass audible");
 uint8_t legacy[652];memcpy(legacy,&p,652);uint32_t v=4;memcpy(legacy+4,&v,4);assert(md_import(legacy,sizeof legacy));assert(md_state()->version==7&&md_state()->fx_level==0);puts("PASS 1.0 migration retains old fields and disables additions");
 p=*md_state();p.fx_level=1001;assert(!md_load(&p));p=*md_state();p.ext[0].notes[15]=25;assert(!md_load(&p));
 p=blank(MD_BASS);p.ext[0].bass_mode=1;for(int &n:p.ext[0].notes)n=-1;assert(md_load(&p));cmd(MD_PLAY,0,1);assert(run(3000)==0&&md_status()->events[0]==0);puts("PASS phrase rests do not trigger");
 p=blank(MD_BASS);p.ext[0].bass_mode=1;p.ext[0].marbles=1000;p.locked=1;assert(hash(p)==hash(p));auto a=hash(p);p.ext[0].marbles=0;assert(a!=hash(p));puts("PASS Marbles remembered variation deterministic and audible");
 p=blank(MD_VA);p.track[0].filter=1;p.track[0].cutoff=500;a=hash(p);p.ext[0].tides=800;assert(a!=hash(p));puts("PASS Tides changes filtered audio");
 p=blank(MD_BASS);a=hash(p);p.ext[0].warps=800;assert(a!=hash(p));puts("PASS Warps ring blend changes audio");
 p=blank(MD_BASS);p.track[0].decay=0;p.ext[0].delay_send=800;p.ext[0].reverb_send=800;p.fx_level=800;p.delay_fb=800;p.reverb_time=850;md_init();assert(md_load(&p));cmd(MD_TRIGGER,0,36);run(300);assert(run(800)>0.001);cmd(MD_PANIC,0,0);assert(run(1000)==0);puts("PASS shared effect tails / panic exact silence");
 for(int quality=0;quality<=1;quality++)for(int model: {MD_BASS,MD_ELEMENTS,MD_VA,MD_RINGS}){p=blank(model);for(int i=0;i<6;i++){p.track[i]=p.track[0];p.ext[i].bass_mode=1;p.ext[i].delay_send=800;p.ext[i].reverb_send=800;p.ext[i].tides=600;p.ext[i].warps=500;p.ext[i].marbles=800;p.ext[i].glide=400;}p.quality=quality;p.fx_level=750;p.delay_fb=800;p.reverb_time=850;p.tempo=220;md_init();assert(md_load(&p));cmd(MD_PLAY,0,1);for(int j=0;j<90;j++){cmd(MD_DELAY_TIME,0,j%2?40:350);cmd(MD_GRAIN_SIZE,0,(j*73)%1001);run(230);}assert(!md_status()->nonfinite);printf("PASS 60-second feature stress model %d\n",model);}
 p=blank(MD_MELLOW);assert(md_load(&p));md_midi(0x90,48,127,0);assert(run(100)>1);md_midi(0x80,48,0,0);run(100);assert(run(300)<.0001);puts("PASS MIDI note off releases Mellow");
 for(int note: {24,48,72,96}){p=blank(MD_SATIN);p.track[0].note=note;p.track[0].tone=950;p.track[0].harm=1000;p.track[0].attack=0;assert(md_load(&p));md_midi(0x90,note,110,0);assert(run(100)>0.01);for(int h=0;h<=1000;h+=10){cmd(MD_HARM,0,h);run(3);}md_midi(0x80,note,0,0);run(100);assert(run(300)<.0001);}puts("PASS Satin morph / high and low notes / MIDI release");
 p=blank(MD_MELLOW);assert(md_load(&p));md_midi(0x90,48,100,0);run(10);md_midi(0xb0,64,127,0);md_midi(0x90,48,0,0);assert(run(100)>1);md_midi(0xb0,64,0,0);run(100);assert(run(300)<.0001);puts("PASS sustain and velocity-zero note off");
 p=blank(MD_MELLOW);assert(md_load(&p));md_midi(0x90,48,40,0);double quiet=run(100);md_init();assert(md_load(&p));md_midi(0x90,48,120,0);double loud=run(100);assert(loud>quiet*7&&loud<quiet*11);md_midi(0xb0,74,32,0);assert(md_state()->track[0].cutoff==32*1000/127);puts("PASS velocity energy and MIDI CC routing");
 p=blank(MD_BASS);p.track[0].hits=0;p.ext[0].bass_mode=2;p.track[0].length=4;for(int &v:p.ext[0].notes)v=-1;p.ext[0].notes[0]=7;assert(md_load(&p));cmd(MD_PLAY,0,1);run(1000);assert(md_status()->events[0]>0&&md_status()->last_note[0]==43);puts("PASS independent Step sequence with Euclidean hits zero");
 md_init();cmd(MD_CLOCK_SOURCE,0,1);md_midi(0xfa,0,0,2);assert(!md_playing());md_midi(0xfa,0,0,0);assert(md_playing());md_midi(0xf8,0,0,0);assert(md_status()->step==0);for(int k=0;k<6;k++)md_midi(0xf8,0,0,0);assert(md_status()->step==1);run(800);assert(!md_playing());puts("PASS selected external clock and loss timeout");
 p=blank(MD_ELEMENTS);for(int i=0;i<6;i++)p.track[i]=p.track[0];assert(md_load(&p));cmd(MD_PLAY,0,1);run(10);assert(md_status()->element_modes==2);for(int i=0;i<6;i++)assert(md_status()->events[i]>0);puts("PASS shared Elements budget keeps six tracks active");
 md_init();md_midi(0xb0,0,1,0);md_midi(0xc0,32,0,0);assert(md_state()->tone_preset[0]==160);md_midi(0xc0,33,0,0);assert(md_state()->tone_preset[0]==161);md_midi(0xbf,16,100,0);assert(md_state()->macro[0]==100*1000/127);puts("PASS program bank retained and global channel macros");
 // Rapid plucks must contain the main impulse, not only the negative primer.
 for(int shape: {334,500,666}){
  p=blank(MD_ELEMENTS);p.track[0].shape=shape;p.track[0].harm=280;
  p.track[0].tone=400;p.track[0].decay=700;p.track[0].ratchet=4;p.tempo=220;
  p.track[0].filter=0;p.macro[3]=0;p.fx_level=0;md_init();assert(md_load(&p));
  cmd(MD_PLAY,0,1);double energy=run(2000);
  printf("Elements rapid pluck shape %d energy %.9f\n",shape,energy);fflush(stdout);
  // Missing main pulses measured 3.46 here; intact plucks exceed 1300.
  assert(energy>100);assert(md_status()->nonfinite==0);
 }
 puts("PASS rapid Plectrum rolls remain audible across the complete shape region");
 {
  // Monades MIDI out: zones (with an overlap), per-track mode, clock, balanced note-offs, no echo of MIDI input.
  uint8_t m[3];int on[16],off[16],clocks,starts,stops;
  auto reset=[&]{memset(on,0,sizeof on);memset(off,0,sizeof off);clocks=starts=stops=0;};
  auto drain=[&]{while(md_midi_take(m)){int st=m[0];if(st==0xf8)clocks++;else if(st==0xfa)starts++;else if(st==0xfc)stops++;else if((st&0xf0)==0x90)on[st&15]++;else if((st&0xf0)==0x80)off[st&15]++;}};
  auto play=[&](const md_midi_routing &r,int blocks){md_patch q=blank(MD_BASS);q.track[0].prob=1000;q.tempo=120;assert(md_load(&q));md_set_midi_routing(&r);drain();reset();
   cmd(MD_PLAY,0,1);for(int b=0;b<blocks;b++){md_render(audio,128);drain();}cmd(MD_PLAY,0,0);drain();};
  md_midi_routing r={};r.mode=2;r.clock=1;r.zone[0].lo=24;r.zone[0].hi=47;r.zone[0].ch=1;r.zone[1].lo=36;r.zone[1].hi=60;r.zone[1].ch=2;r.zone[2].lo=72;r.zone[2].hi=96;r.zone[2].ch=3;
  play(r,689);
  assert(starts==1&&stops==1);assert(clocks>=95&&clocks<=98);
  assert(on[0]>=15&&on[0]==on[1]&&on[2]==0);for(int c=0;c<16;c++)assert(on[c]==off[c]);
  puts("PASS MIDI out zones: overlap sends on both channels, clock 24 ppqn, start/stop, every note released");
  md_midi_routing t1={};t1.mode=1;play(t1,689);
  assert(on[0]>=15&&clocks==0&&starts==0);for(int c=1;c<16;c++)assert(on[c]==0);for(int c=0;c<16;c++)assert(on[c]==off[c]);
  puts("PASS MIDI out per track: track 1 on channel 1, no clock when disabled");
  md_midi_routing none={};play(none,300);for(int c=0;c<16;c++)assert(on[c]==0);assert(clocks==0);
  md_set_midi_routing(&t1);md_midi(0x90,60,100,0);run(10);md_midi(0x80,60,0,0);run(10);drain();for(int c=0;c<16;c++)assert(on[c]==0);
  puts("PASS MIDI out off sends nothing; notes received over MIDI are never echoed");
 }
 {
  // Monades scales: 1.1 indices unchanged, raga ascent / descent, custom scale, browsing order covers every scale.
  const unsigned legacy[8]={0xfff,0x5ad,0xab5,0x6ad,0x4a9,0x295,0x555,0x5ab};
  for(int i=0;i<8;i++)assert(md_scale_mask(i)==legacy[i]&&md_scale_mask_down(i)==legacy[i]);
  int khamaj=-1;for(int i=0;i<md_scale_count();i++)if(!strcmp(md_scale_name(i),"KHAMAJ"))khamaj=i;assert(khamaj>0);
  assert(md_scale_mask(khamaj)==((1u<<0)|(1u<<4)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<11)));
  assert(md_scale_mask_down(khamaj)==((1u<<0)|(1u<<2)|(1u<<4)|(1u<<5)|(1u<<7)|(1u<<9)|(1u<<10)));
  md_set_custom_scale(0x004);assert(md_scale_mask(md_scale_custom())==0x005);
  assert(md_quantize(63,0,md_scale_custom())==62);
  bool seen[128]={};for(int k=0;k<md_scale_count();k++){int sc=md_scale_at(k);assert(!seen[sc]);seen[sc]=true;assert(md_scale_position(sc)==k);}
  md_patch q=blank(MD_BASS);q.scale=md_scale_count()-1;assert(md_validate(&q));q.scale=md_scale_count();assert(!md_validate(&q));
  printf("PASS %d scales: 1.1 order kept, raga ascent/descent, custom scale, browsing order complete\n",md_scale_count());
 }
 {
  // Monades microtonal: cents quantizing, non-octave periods, MIDI pitch bend in per-track mode only.
  auto find=[](const char *n){for(int i=0;i<md_scale_count();i++)if(!strcmp(md_scale_name(i),n))return i;return -1;};
  int rast=find("RAST"),bp=find("BOHLEN PIERCE"),shruti=find("SHRUTI22");assert(rast>0&&bp>0&&shruti>0);
  assert(fabsf(md_quantize_pitch(63.4f,0,rast)-63.5f)<1e-4f);           /* E half-flat, 350 cents above C */
  assert(fabsf(md_quantize_pitch(70.4f,0,rast)-70.5f)<1e-4f);           /* B half-flat */
  float d[32],per=0;assert(md_scale_cents(bp,d,32,&per)==13&&fabsf(per-1901.955f)<.01f);
  {float lattice=(4*1901.955f+292.6f)*.01f;assert(fabsf(md_quantize_pitch(lattice+.3f,0,bp)-lattice)<.01f);} /* 4 tritaves + 2 steps above the root */
  assert(md_scale_cents(shruti,d,32,&per)==22&&per==1200);
  for(int i=0;i<md_scale_count();i++){float q=md_quantize_pitch(61.3f,2,i);assert(q>=24&&q<=96);}
  int bends=0,on=0;uint8_t m[3];
  md_patch q=blank(MD_VA);q.scale=rast;q.track[0].note=63;q.track[0].prob=1000;assert(md_load(&q));
  md_midi_routing r={};r.mode=1;md_set_midi_routing(&r);while(md_midi_take(m));
  cmd(MD_PLAY,0,1);for(int b=0;b<300;b++){md_render(audio,128);while(md_midi_take(m)){if((m[0]&0xf0)==0xe0)bends++;if((m[0]&0xf0)==0x90)on++;}}cmd(MD_PLAY,0,0);
  assert(on>0&&bends>0);
  r.mode=2;r.zone[0].lo=24;r.zone[0].hi=96;r.zone[0].ch=1;md_set_midi_routing(&r);while(md_midi_take(m));bends=on=0;
  cmd(MD_PLAY,0,1);for(int b=0;b<300;b++){md_render(audio,128);while(md_midi_take(m)){if((m[0]&0xf0)==0xe0)bends++;if((m[0]&0xf0)==0x90)on++;}}cmd(MD_PLAY,0,0);
  assert(on>0&&bends==0);
  puts("PASS microtonal: Rast quarter tones, Bohlen-Pierce tritave, 22 shrutis, pitch bend per track, nearest note in zones");
 }
 {
  // Patch v7: a Mutable Dreams 1.1 patch (v6, 1292 bytes) loads with every Monades addition neutral.
  md_patch q=blank(MD_VA);uint8_t old[1292];memcpy(old,&q,1292);uint32_t v6=6;memcpy(old+4,&v6,4);
  assert(md_import(old,sizeof old));const md_patch *s=md_state();assert(s->version==7&&s->send_hp==0&&s->echo_tone==500&&s->room_tone==500);
  for(int i=0;i<6;i++)for(int k=0;k<2;k++)assert(s->lfo[i][k].depth==0);
  uint32_t v7=7;memcpy(old+4,&v7,4);assert(!md_import(old,sizeof old));
  q=blank(MD_VA);q.lfo[2][1].rate=MD_LFO_RATES;assert(!md_validate(&q));q=blank(MD_VA);q.lfo[0][0].depth=1001;assert(!md_validate(&q));q=blank(MD_VA);q.track[0].filter=MD_FILTER_MODES;assert(!md_validate(&q));
  // Synced LFO: a 1/4 cycle starts again exactly every four steps, from PLAY.
  q=blank(MD_VA);q.tempo=120;q.lfo[0][0]={7,MD_LFO_SINE,MD_TO_CUTOFF,500};assert(md_load(&q));cmd(MD_PLAY,0,1);
  bool seen=false;for(int b=0;b<400&&!seen;b++){md_render(audio,128);if(md_status()->step==8){seen=true;assert(md_status()->lfo_cycle[0][0]==2&&md_status()->lfo_phase[0][0]<.02f);}}
  assert(seen);cmd(MD_PLAY,0,0);
  // Square LFO on the cutoff: the filter really moves; with depth 0 it does not.
  auto swing=[&](int depth){md_patch r=blank(MD_VA);r.track[0].filter=1;r.track[0].cutoff=500;r.track[0].breathe=0;r.ext[0].tides=0;r.lfo[0][0]={10,MD_LFO_SQUARE,MD_TO_CUTOFF,depth};assert(md_load(&r));cmd(MD_PLAY,0,1);
   float lo=1e9f,hi=0;for(int b=0;b<600;b++){md_render(audio,128);if(b>100){float c=md_status()->cutoff_now[0];lo=fminf(lo,c);hi=fmaxf(hi,c);}}cmd(MD_PLAY,0,0);return hi/lo;};
  assert(swing(0)<1.01f&&swing(1000)>8);
  // Sample & hold: one value per cycle, the one the screen draws, and it changes from cycle to cycle.
  q=blank(MD_VA);q.lfo[1][0]={14,MD_LFO_SAMPLE_HOLD,MD_TO_PITCH,300};assert(md_load(&q));cmd(MD_PLAY,0,1);int changes=0;float last=2;
  for(int b=0;b<600;b++){md_render(audio,128);const md_stats *m=md_status();float v=m->lfo_value[1][0];assert(fabsf(v-md_lfo_wave(1,0,m->lfo_cycle[1][0],.7f))<1e-6f);if(v!=last)changes++;last=v;}
  cmd(MD_PLAY,0,0);assert(changes>10);
  // Tremolo never raises the level.
  auto peak=[&](int depth){md_patch r=blank(MD_VA);r.lfo[0][0]={10,MD_LFO_SINE,MD_TO_LEVEL,depth};assert(md_load(&r));cmd(MD_PLAY,0,1);float pk=0;for(int b=0;b<500;b++){md_render(audio,128);pk=fmaxf(pk,md_status()->track_peak[0]);}cmd(MD_PLAY,0,0);return pk;};
  {float flat=peak(0);assert(peak(1000)<=flat*1.02f&&peak(-1000)<=flat*1.02f);}
  puts("PASS LFOs: 1.1 patches migrate, synced phase restarts with PLAY, cutoff moves, sample & hold matches the view, tremolo never boosts");
  // Filter modes: high-pass thins a low note, band and notch stay finite and audible.
  auto energy=[&](int mode,int cutoff){md_patch r=blank(MD_VA);r.track[0].filter=mode;r.track[0].cutoff=cutoff;r.track[0].resonance=0;r.track[0].breathe=0;assert(md_load(&r));cmd(MD_PLAY,0,1);double e=run(400);cmd(MD_PLAY,0,0);return e;};
  {double dry=energy(0,500);assert(energy(4,850)<dry*.5);assert(energy(5,600)>0&&energy(6,600)>0&&energy(6,600)<dry*1.2);}
  // Send high-pass: a kick through the echo loses most of its body.
  auto echo=[&](int hp){md_patch r=blank(MD_KICK);r.ext[0].delay_send=1000;r.fx_level=1000;r.send_hp=hp;assert(md_load(&r));cmd(MD_PLAY,0,1);float pk=0;for(int b=0;b<500;b++){md_render(audio,128);pk=fmaxf(pk,md_status()->fx_peak[1]);}cmd(MD_PLAY,0,0);return pk;};
  {float full=echo(0);assert(full>0&&echo(1000)<full*.5f);}
  assert(md_send_hp_hz(0)==0&&md_send_hp_hz(1)>=20&&md_send_hp_hz(1000)==2000);
  puts("PASS filters: HP / BP / notch, 12 dB send high-pass on the echo and room");
 }
 puts("ALL NEXT FEATURE TESTS PASSED; no hardware-performance claim");
}
