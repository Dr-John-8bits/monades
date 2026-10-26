/* SPDX-License-Identifier: MIT OR GPL-3.0-only
 * Mutable Dreams. Original Mutable Instruments DSP remains under its MIT notices.
 * Six interchangeable monophonic slots. No allocations in render. */
#include "dreams.h"
#include "elements/dsp/exciter.h"
#include "elements/dsp/resonator.h"
#include <math.h>
#include <string.h>
#include <new>
#include <stddef.h>
#include "plaits/dsp/engine/fm_engine.h"
#include "plaits/dsp/engine/virtual_analog_engine.h"
#include "plaits/dsp/drums/analog_bass_drum.h"
#include "plaits/dsp/drums/analog_snare_drum.h"
#include "plaits/dsp/drums/hi_hat.h"
#include "rings/dsp/resonator.h"
#include "plaits/dsp/engine/additive_engine.h"
#include "plaits/dsp/oscillator/sine_oscillator.h"
#include "stmlib/dsp/filter.h"
#include "stmlib/utils/random.h"
#ifndef PLAT_HOST
#define POOL __attribute__((section(".pool")))
#else
#define POOL
#endif
namespace {
#include "satin_tables.inc"
constexpr unsigned Q=64, GN=131072, GM=GN-1;
struct Voice {
 plaits::AdditiveEngine additive; plaits::SineOscillator choir[3]; stmlib::Svf filter1,filter2;
 plaits::FMEngine fm; plaits::VirtualAnalogEngine va;
 plaits::AnalogBassDrum kick; plaits::AnalogSnareDrum snare;
 plaits::HiHat<plaits::SquareNoise,plaits::SwingVCA,true,false> hat;
 rings::Resonator rings;
 float satin_morph,satin_tone;
 float expression_smooth,pan_smooth,midi_gate,mellow_phase,mellow_drift;
 float strike_gate,target_note,sub_phase,tide_phase,warp_phase,warp_smooth,delay_send_smooth,reverb_send_smooth;
 float mem[112],env,amp,note,excite,fade,velocity,cut_smooth,res_smooth,breath_phase,lpg,eco_previous,eco_next;
 int model,trigger,attacking,active,eco_phase;
};
alignas(Voice) static unsigned char voices_raw[sizeof(Voice)*6] POOL;
static Voice *voices;
// Strike resonators use ordinary RAM; the large tape/effect pool stays bounded.
static elements::Resonator element_body[6];
static elements::Exciter element_strike[6];
static int16_t tape[GN] POOL;
// Independent effect storage: never borrow or shorten the granular tape.
static int16_t echo[16384] POOL;
static int16_t room[4][1201];
static bool fx_active;
static unsigned echo_head,room_head[4];
static float echo_lp,room_lp[4],fx_smooth,delay_smooth=8820,fx_highpass_in,fx_highpass_out;
static void reset_fx(){fx_active=false;memset(echo,0,sizeof echo);memset(room,0,sizeof room);memset(room_head,0,sizeof room_head);memset(room_lp,0,sizeof room_lp);echo_head=0;echo_lp=fx_smooth=fx_highpass_in=fx_highpass_out=0;delay_smooth=8820;}
static void extension_defaults(md_patch &p){p.version=6;p.delay_ms=200;p.delay_fb=300;p.reverb_time=450;p.fx_level=0;memset(p.ext,0,sizeof p.ext);for(int i=0;i<6;i++){p.ext[i].tides_rate=250;for(int j=0;j<16;j++)p.ext[i].notes[j]=(j%4==0?0:j%4==2?7:-1);}}

struct Grain { float pos,phase,inc,pan,rate,gain; unsigned life,age; };
struct Repeat { float left,spacing,note,velocity; unsigned remaining; };
static Repeat repeats[6];
static Grain grains[4];
static md_patch patch,previous;
static md_stats stats;
static unsigned writehead,grainclock,tape_valid;
static float feedback_lp,dcx[2],dcy[2],smooth[4];
static unsigned rng,seqrng,step;
static float untilstep,duckenv;
static int playing;
static int clock_source,midi_quantize,clock_pulse;
static unsigned clock_age,clock_last;static float clock_interval;
static int held[6],sustain[6],midi_bank[6];
static float bend[6],bend_smooth[6],expression[6];
struct Command { int c,t,v; };
static Command queue[64];
static volatile unsigned qread,qwrite;
static inline void fence() { __asm__ volatile("" ::: "memory"); }
static int bound(int x,int a,int b) { return x<a?a:x>b?b:x; }
static float unit(int x) { return bound(x,0,1000)*.001f; }
static unsigned random_word(unsigned &r) { r^=r<<13;r^=r>>17;r^=r<<5;return r; }
static float noise() { return (random_word(rng)>>8)*(1.0f/8388608.0f)-1.0f; }
// Linear at ordinary levels; only excess peaks enter the soft safety knee.
static float clip(float x) { float a=fabsf(x);if(a<=.8f)return x;float y=.8f+.2f*(a-.8f)/(.2f+a-.8f);return x<0?-y:y; }
// Keep existing warm settings up to 90%; open the final tenth to 18 kHz.
static float filter_q(float resonance) { return .707f+2.0f*resonance*resonance; }
static float cutoff_hz(float u) { return 60.0f*stmlib::SemitonesToRatio(u*84.0f+fmaxf(0.0f,u-.9f)*147.458f); }
static float tape_at(unsigned i){i&=GM;return (i<tape_valid?tape[i]:0)*(1.0f/32768.0f);}
static float tape_read(float pos){
 unsigned a=(unsigned)pos;float t=pos-a;
 float xm=tape_at(a-1),x0=tape_at(a),x1=tape_at(a+1),x2=tape_at(a+2);
 // Four-point Hermite interpolation: exact at stored samples, no linear droop.
 float c=.5f*(x1-xm),b=xm-2.5f*x0+2*x1-.5f*x2,d=.5f*(x2-xm)+1.5f*(x0-x1);
 return ((d*t+b)*t+c)*t+x0;
}
static void reset_voice(unsigned i) {
 memset(static_cast<void*>(&voices[i]),0,sizeof(Voice));
 Voice *v=new (&voices[i]) Voice();
 stmlib::BufferAllocator a(v->mem,sizeof v->mem);
 v->fm.Init(&a); v->va.Init(&a);v->kick.Init();v->snare.Init();v->hat.Init();v->rings.Init();
 v->additive.Init(&a);v->additive.Reset();v->filter1.Init();v->filter2.Init();for(int k=0;k<3;k++)v->choir[k].Init();
 v->cut_smooth=patch.track[i].cutoff*.001f;v->res_smooth=patch.track[i].resonance*.001f;v->breath_phase=i*.137f;
 v->expression_smooth=1;v->pan_smooth=unit(patch.track[i].pan);held[i]=-1;sustain[i]=0;bend[i]=bend_smooth[i]=0;expression[i]=1;
 v->model=patch.track[i].model;v->note=(float)patch.track[i].note;v->target_note=v->note;v->fade=0;v->velocity=.8f;
 memset(static_cast<void*>(&element_body[i]),0,sizeof(element_body[i]));element_body[i].Init();element_body[i].set_modulation_frequency(.5f/MD_RATE);element_body[i].set_modulation_offset(.25f);
 memset(static_cast<void*>(&element_strike[i]),0,sizeof(element_strike[i]));element_strike[i].Init();
 repeats[i].remaining=0;
}
#include "preset_bank.inc"
static void clear_audio() {
 reset_fx();tape_valid=0;writehead=grainclock=0;
 memset(grains,0,sizeof grains);memset(repeats,0,sizeof repeats);
 memset(dcx,0,sizeof dcx);memset(dcy,0,sizeof dcy);feedback_lp=duckenv=0;
}
static void scene(int id) {
 patch=biome_bank[bound(id,0,MD_BIOMES-1)].patch;extension_defaults(patch);
 if(id>=65){patch.fx_level=340;patch.reverb_time=550;for(int i=0;i<6;i++)patch.ext[i].reverb_send=300;}
 clear_audio();for(int i=0;i<6;i++){reset_voice(i);}for(int j=0;j<4;j++)smooth[j]=unit(patch.macro[j]);
 previous=patch;step=0;untilstep=0;seqrng=patch.seed;
}
static void tone_preset(int track,int id) {
 id=bound(id,0,MD_TONE_PRESETS-1);const TonePreset &a=tone_bank[id];md_track &t=patch.track[track];
 t.model=a.model;t.tone=a.tone;t.harm=a.harm;t.decay=a.decay;t.attack=a.attack;t.shape=a.shape;
 t.cutoff=a.cutoff;t.resonance=a.resonance;t.filter=a.filter;t.breathe=a.breathe;
 patch.tone_preset[track]=id;patch.edited_mask=(patch.edited_mask&~(2u<<track))|1u;reset_voice(track);
}
static int32_t *track_field(md_track &t,int c) {
switch(c){case MD_MODEL:return &t.model;case MD_TONE:return &t.tone;case MD_HARM:return &t.harm;case MD_DECAY:return &t.decay;case MD_LEVEL:return &t.level;case MD_SEND:return &t.send;case MD_HITS:return &t.hits;case MD_LENGTH:return &t.length;case MD_ROTATE:return &t.rotate;case MD_PROB:return &t.prob;case MD_NOTE:return &t.note;case MD_PAN:return &t.pan;case MD_MUTE:return &t.mute;default:return &t.speed;}
}
static void fire(unsigned i,float note,float velocity) {
 Voice &v=voices[i];v.trigger=1;v.midi_gate=0;v.target_note=note;if(!patch.ext[i].glide||!v.active)v.note=note;v.velocity=velocity;v.active=1;
 stats.events[i]++;stats.last_note[i]=(int)note;
 if((int)i==patch.duck_source&&!patch.track[i].mute)duckenv=1;
}
static void apply(Command e) {
 int i=bound(e.t,0,5),v=e.v;md_track &t=patch.track[i];
 // Playback, preview notes and emergency silence do not change a preset.
 if(e.c!=MD_SCENE&&e.c!=MD_RETURN&&e.c!=MD_PLAY&&e.c!=MD_TRIGGER&&e.c!=MD_PANIC&&e.c!=MD_MUTATE)patch.edited_mask|=1u;
 if(e.c==MD_MODEL||e.c==MD_TONE||e.c==MD_HARM||e.c==MD_DECAY||e.c==MD_ATTACK||(e.c>=MD_SHAPE&&e.c<=MD_BREATHE))patch.edited_mask|=2u<<i;
 if(e.c<=MD_SPEED) {
  int32_t *p=track_field(t,e.c);
  if(e.c==MD_MODEL) { *p=bound(v,0,MD_MODELS-1);patch.tone_preset[i]=-1;reset_voice(i); }
  else if(e.c==MD_LENGTH) { t.length=bound(v,1,32);t.hits=bound(t.hits,0,t.length);t.rotate%=t.length; }
  else if(e.c==MD_HITS) t.hits=bound(v,0,t.length);
  else if(e.c==MD_ROTATE) t.rotate=bound(v,0,t.length-1);
  else if(e.c==MD_NOTE) t.note=bound(v,24,96);
  else if(e.c==MD_MUTE) t.mute=!!v;
  else if(e.c==MD_SPEED) t.speed=bound(v,1,8);
  else *p=bound(v,0,1000);
 } else switch(e.c) {
 case MD_MACRO:patch.macro[bound(e.t,0,3)]=bound(v,0,1000);break;
 case MD_TEMPO:patch.tempo=bound(v,30,220);break;
 case MD_PLAY:playing=!!v;memset(repeats,0,sizeof repeats);if(playing){step=0;untilstep=0;seqrng=patch.seed;}break;
 case MD_LOCK:patch.locked=!!v;break;
 case MD_MUTATE: previous=patch;patch.edited_mask|=1u;patch.seed=random_word(seqrng)|1;for(unsigned j=0;j<6;j++)patch.track[j].rotate=random_word(seqrng)%patch.track[j].length;break;
 case MD_RETURN:patch=previous;seqrng=patch.seed;for(unsigned j=0;j<6;j++){repeats[j].remaining=0;if(voices[j].model!=patch.track[j].model)reset_voice(j);}break;
 case MD_SCENE:scene(v);break;
 case MD_TONE_PRESET:tone_preset(i,v);break;
 case MD_FREEZE:patch.freeze=!!v;break;
 case MD_GRAIN_SIZE:patch.grain_size=bound(v,0,1000);break;
 case MD_GRAIN_POS:patch.grain_pos=bound(v,0,1000);break;
 case MD_FEEDBACK:patch.feedback=bound(v,0,850);break;
 case MD_QUALITY:if(patch.quality!=bound(v,0,1)){patch.quality=bound(v,0,1);for(unsigned j=0;j<6;j++){voices[j].eco_phase=0;voices[j].eco_previous=voices[j].eco_next=0;voices[j].fade=0;}}break;
 case MD_TRIGGER:fire(i,(float)(t.model>=MD_FM?md_quantize(v+patch.transpose,patch.root,patch.scale):bound(v,24,96)),.8f);break;
 case MD_ATTACK:t.attack=bound(v,0,1000);break;
 case MD_RATCHET:t.ratchet=bound(v,1,4);break;
 case MD_SHAPE:t.shape=bound(v,0,1000);break;
 case MD_CUTOFF:t.cutoff=bound(v,0,1000);break;
 case MD_RESONANCE:t.resonance=bound(v,0,1000);break;
 case MD_FILTER:if(t.filter!=bound(v,0,3)){v=bound(v,0,3);t.filter=v;voices[i].filter1.Reset();voices[i].filter2.Reset();voices[i].fade=0;}break;
 case MD_BREATHE:t.breathe=bound(v,0,1000);break;
 case MD_ACCENT:t.accent=bound(v,0,1000);break;
 case MD_SWING:patch.swing=bound(v,0,600);break;
 case MD_ROOT:patch.root=bound(v,0,11);break;
 case MD_SCALE:patch.scale=bound(v,0,7);break;
 case MD_TRANSPOSE:patch.transpose=bound(v,-24,24);break;
 case MD_GRAIN_PITCH:patch.grain_pitch=bound(v,-24,24);break;
 case MD_REVERSE:patch.reverse=!!v;break;
 case MD_DUCK_SOURCE:patch.duck_source=bound(v,-1,5);break;
 case MD_DUCK:patch.duck_amount=bound(v,0,1000);break;
 case MD_DUCK_RELEASE:patch.duck_release=bound(v,0,1000);break;
 case MD_DELAY_SEND:patch.ext[i].delay_send=bound(v,0,1000);break;
 case MD_REVERB_SEND:patch.ext[i].reverb_send=bound(v,0,1000);break;
 case MD_DELAY_TIME:patch.delay_ms=bound(v,40,350);break;
 case MD_DELAY_FB:patch.delay_fb=bound(v,0,800);break;
 case MD_REVERB_TIME:patch.reverb_time=bound(v,0,850);break;
 case MD_FX_LEVEL:patch.fx_level=bound(v,0,1000);break;
 case MD_CLOCK_SOURCE:clock_source=bound(v,0,2);playing=0;clock_pulse=0;clock_age=clock_last=0;clock_interval=0;break;
 case MD_MIDI_QUANTIZE:midi_quantize=!!v;break;
 case MD_BASS_MODE:patch.ext[i].bass_mode=bound(v,0,2);break;
 case MD_BASS_PATTERN:{static const int motifs[4][16]={{0,-1,0,-1,7,-1,0,-1,0,-1,3,-1,7,-1,10,-1},{0,0,-1,7,0,-1,3,-1,0,0,7,-1,10,-1,7,-1},{0,-1,-1,-1,0,-1,7,-1,3,-1,-1,-1,7,-1,5,-1},{0,-1,12,7,0,3,-1,7,0,-1,10,7,5,-1,3,-1}};patch.ext[i].pattern=bound(v,0,3);memcpy(patch.ext[i].notes,motifs[patch.ext[i].pattern],sizeof motifs[0]);break;}
 case MD_GLIDE:patch.ext[i].glide=bound(v,0,1000);break;
 case MD_MARBLES:patch.ext[i].marbles=bound(v,0,1000);break;
 case MD_TIDES:patch.ext[i].tides=bound(v,0,1000);break;
 case MD_TIDES_RATE:patch.ext[i].tides_rate=bound(v,0,1000);break;
 case MD_WARPS:patch.ext[i].warps=bound(v,0,1000);break;
 case MD_BASS_STEP:patch.ext[i].edit_step=bound(v,0,15);break;
 case MD_BASS_VALUE:patch.ext[i].notes[patch.ext[i].edit_step]=bound(v,-1,24);break;
 case MD_PANIC:playing=0;clear_audio();for(unsigned j=0;j<6;j++)reset_voice(j);break;
 }
}
static void tick() {
 float duration=MD_RATE*60.0f/(patch.tempo*4.0f)*(1+(step%2?-1:1)*unit(patch.swing)*.5f);
 if((step%32)==0) {if(patch.locked)seqrng=patch.seed;else patch.seed=random_word(seqrng)|1;}
 static const int degree[7]={0,2,3,5,7,10,12};
 for(unsigned i=0;i<6;i++) {
  const md_track &t=patch.track[i];if(step%(unsigned)t.speed)continue;
  unsigned local=step/t.speed;
  int hits=bound(t.hits+(patch.macro[0]-500)*t.length/1500,0,t.length);
  unsigned roll=random_word(seqrng);
  if((patch.ext[i].bass_mode==2||md_hit(local,hits,t.length,t.rotate))&&roll%1000<(unsigned)t.prob&&!t.mute) {
   int note=t.note;
   const md_extension &e=patch.ext[i];
   if(e.bass_mode){int off=e.notes[local%(e.bass_mode==2?(t.length>16?16:t.length):16)];if(off<0)continue;note+=off;
    // Marbles-inspired remembered variation: deterministic for each seed/position,
    // independent of audio noise and stable across repeated phrases when locked.
    unsigned mr=patch.seed^(i*0x9e3779b9u)^((local%16+1)*0x85ebca6bu);unsigned choice=random_word(mr);
    if((local%4)!=0&&choice%1000<(unsigned)e.marbles){static const int interval[]={0,3,5,7};note=t.note+interval[(choice>>12)&3];}
   }
   if(t.model>=MD_FM) {
    if(!e.bass_mode&&(roll>>10)%1000<(unsigned)patch.macro[1])note+=degree[(roll>>20)%7];
    note=md_quantize(note+patch.transpose,patch.root,patch.scale);
   }
   float velocity=.8f+unit(t.accent)*((local%t.length)==0?.2f:-.25f);
   fire(i,(float)note,velocity);
   repeats[i]={(duration/t.ratchet),duration/t.ratchet,(float)note,velocity,(unsigned)t.ratchet-1};
  }
 }
 stats.step=step++;untilstep+=duration;
}
static void quantum(float *lr,unsigned n) {
 // Conservative admission guard from 0.4.1 FM-1 costs with full-rate tape, LP24 and maximum repeats.
 // Count unmuted slots before playback so the tape cannot prefill four FULL grains
 // immediately before a dense six-voice attack. Existing audible tails also count.
 if(patch.quality){static const unsigned weight[MD_MODELS]={8,13,8,18,10,26,17,8,8,30,10,12};unsigned cost=12;
  // Provisional additional weights, pending physical-device measurement.
  if(fx_active||patch.fx_level)cost+=10;
  for(unsigned j=0;j<6;j++)if(!patch.track[j].mute&&patch.ext[j].warps)cost+=2;
  for(unsigned i=0;i<6;i++)if(voices[i].active||!patch.track[i].mute)cost+=weight[patch.track[i].model];
  if(cost>85){patch.quality=0;stats.quality_reductions++;for(unsigned i=0;i<6;i++){voices[i].eco_phase=0;voices[i].eco_previous=voices[i].eco_next=0;voices[i].fade=0;}}
 }

 // A shared mode budget also applies in ECO. All six tracks remain audible.
 unsigned element_count=0;
 for(unsigned j=0;j<6;j++)if(patch.track[j].model==MD_ELEMENTS&&(voices[j].active||!patch.track[j].mute))element_count++;
 unsigned element_modes=element_count>1?16/element_count:(patch.quality?16:8);
 element_modes=element_modes<2?2:element_modes;stats.element_modes=element_modes;
 const float block_ratio=n>32?n/32.0f:1.0f;
 const float slew25=n>32?.025f*block_ratio*(1-.0125f*(block_ratio-1)):.025f;
 const float slew40=n>32?.04f*block_ratio*(1-.02f*(block_ratio-1)):.04f;
 const float slew35=n>32?.035f*block_ratio*(1-.0175f*(block_ratio-1)):.035f;
 for(unsigned i=0;i<6;i++)stats.track_peak[i]*=n>32?.984064f:.992f;
 for(unsigned j=0;j<4;j++)smooth[j]+=(unit(patch.macro[j])-smooth[j])*slew25;
 float delay_send[Q]={},room_send[Q]={};
 float send[Q]={},out[Q],aux[Q],tmp[Q],exc[Q],duck[Q];memset(lr,0,n*2*sizeof(float));
 float duckdec=1.0f-1.0f/(MD_RATE*(.04f+unit(patch.duck_release)*1.46f));
 for(unsigned k=0;k<n;k++){duck[k]=1-duckenv*unit(patch.duck_amount)*.9f;duckenv*=duckdec;}
 float grainrate=stmlib::SemitonesToRatio((float)patch.grain_pitch)*(patch.reverse?-1:1);
 float fb=unit(patch.feedback);
 for(unsigned i=0;i<6;i++) {
  md_track &t=patch.track[i];Voice &v=voices[i];bool trig=v.trigger;v.trigger=0;
  if(trig) {v.active=1;v.attacking=t.attack>0; if(!v.attacking)v.env=1;v.excite=1;v.lpg=1;}
  /* Only FM/VA have an observable final envelope. Never cut resonator/drum tails. */
  if(!v.active){stats.skipped_quanta++;continue;}stats.voice_quanta++;
  const md_extension &e=patch.ext[i];
  v.note+=(v.target_note-v.note)*(e.glide?n/(float)(n+MD_RATE*.35f*unit(e.glide)):1.0f);
  v.tide_phase+=n/(MD_RATE*(.5f+unit(e.tides_rate)*19.5f));if(v.tide_phase>=1)v.tide_phase-=1;
  float tidal=(1.0f-4.0f*fabsf(v.tide_phase-.5f))*unit(e.tides);
  float tone=fminf(.98f,fmaxf(t.model==MD_HARMONIC?0.0f:.02f,unit(t.tone)+(smooth[2]-.5f)*.65f));
  bend_smooth[i]+=(bend[i]-bend_smooth[i])*slew40;
  float played_note=v.note+bend_smooth[i];
  float f=440.0f*stmlib::SemitonesToRatio(played_note-69.0f)/MD_RATE;
  plaits::EngineParameters p={trig?1:0,played_note,tone,unit(t.shape),unit(t.harm)*.999f,v.velocity};bool done=false;
  memset(out,0,sizeof out);memset(aux,0,sizeof aux);
  bool eco=patch.quality==0&&(t.model==MD_FM||t.model==MD_VA||t.model==MD_HARMONIC);
  unsigned engine_n=eco?(n+(v.eco_phase?0:1))/2:n;
  if(eco)p.note+=12.0f; // same concert pitch at 22.05 kHz; linear reconstruction below.
  switch(t.model) {
   case MD_KICK:v.kick.Render(false,trig,v.velocity,f,tone,unit(t.decay),unit(t.harm),.2f,out,n);break;
   case MD_SNARE:v.snare.Render(false,trig,v.velocity,f,tone,unit(t.decay),unit(t.harm),out,n);break;
   case MD_HAT:v.hat.Render(false,trig,v.velocity,f,tone,unit(t.decay),unit(t.harm),tmp,exc,out,n);break;
   case MD_FM:if(engine_n)v.fm.Render(p,out,nullptr,engine_n,&done);break;
   case MD_VA:if(engine_n)v.va.Render(p,out,nullptr,engine_n,&done);break;
   case MD_HARMONIC:if(engine_n)v.additive.Render(p,out,nullptr,engine_n,&done);break;
   case MD_ELEMENTS:{
    elements::Exciter &strike=element_strike[i];elements::Resonator &body=element_body[i];
    if(trig)v.strike_gate=MD_RATE*(.03f+.5f*unit(t.shape));
    unsigned strike_flags=(trig?elements::EXCITER_FLAG_RISING_EDGE:0)|(v.strike_gate>0?elements::EXCITER_FLAG_GATE:0);v.strike_gate=fmaxf(0,v.strike_gate-n);
    strike.set_model(t.shape<334?elements::EXCITER_MODEL_MALLET:t.shape<667?elements::EXCITER_MODEL_PLECTRUM:elements::EXCITER_MODEL_PARTICLES);
    // In the Plectrum region, keep its main pulse 1.7-5.2 ms after the primer.
    // The original 14-38 ms mapping was continually restarted by 220 BPM
    // four-ratchet rolls (17 ms spacing), leaving only the tiny primer audible.
    strike.set_parameter(t.shape>=334&&t.shape<667?.05f+.15f*(t.shape-334)/332.0f:.1f+.8f*unit(t.shape));
    strike.set_timbre(fminf(.95f,tone));strike.Process(strike_flags,exc,n);
    body.set_frequency(f);body.set_geometry(fminf(.999f,unit(t.harm)));body.set_brightness(tone);body.set_damping(unit(t.decay)*.8f);body.set_position(.3f);body.set_resolution(element_modes);
    memset(tmp,0,sizeof tmp);body.Process(tmp,exc,out,aux,n);break;}
   case MD_SATIN:{
    // Original rounded wavetable oscillator. Reduce harmonics continuously
    // before Nyquist; interpolate phase and waveform, with no oversampling RAM.
    v.satin_morph+=(unit(t.harm)*2.999f-v.satin_morph)*slew35;
    v.satin_tone+=(tone-v.satin_tone)*slew35;
    int shape=(int)v.satin_morph;float morph=v.satin_morph-shape;
    v.mellow_drift+=n/(MD_RATE*(9.0f+i));if(v.mellow_drift>=1)v.mellow_drift-=1;
    float f2=f*stmlib::SemitonesToRatio(.13f*unit(t.shape)*plaits::Sine(v.mellow_drift));
    int band=3;float highest=fmaxf(f,f2);
    while(band>0&&highest*(1<<band)>.45f)band--;
    float fade=band?fminf(1.0f,fmaxf(0.0f,(.45f-highest*(1<<band))/.1125f)):1;
    for(unsigned k=0;k<n;k++){
     v.sub_phase+=f;if(v.sub_phase>=1)v.sub_phase-=1;
     v.mellow_phase+=f2;if(v.mellow_phase>=1)v.mellow_phase-=1;
     float sum=0;float phases[2]={v.sub_phase,v.mellow_phase};
     for(int osc=0;osc<2;osc++){
      float index=phases[osc]*256;int at=(int)index;float frac=index-at;
      const float *a=satin_table[band][shape],*b=satin_table[band][shape+1];
      float x=a[at]+(a[at+1]-a[at])*frac,y=b[at]+(b[at+1]-b[at])*frac;
      float wave=x+(y-x)*morph;
      if(band&&fade<1){a=satin_table[band-1][shape];b=satin_table[band-1][shape+1];x=a[at]+(a[at+1]-a[at])*frac;y=b[at]+(b[at+1]-b[at])*frac;float low=x+(y-x)*morph;wave=low+(wave-low)*fade;}
      sum+=wave;
     }
     float fundamental=plaits::Sine(v.sub_phase);
     out[k]=.76f*fundamental+(sum*.42f-.76f*fundamental)*v.satin_tone;
    }break;}
   case MD_MELLOW:{
    // Finite odd-harmonic triangle blend: no discontinuous saw/square aliasing.
    // Small, slow independent drift; Tone adds rounded harmonics, Shape detune.
    v.mellow_drift+=n/(MD_RATE*(8.0f+i));if(v.mellow_drift>=1)v.mellow_drift-=1;
    float detune=stmlib::SemitonesToRatio(.10f*unit(t.shape)*plaits::Sine(v.mellow_drift));
    for(unsigned k=0;k<n;k++){
     v.sub_phase+=f;if(v.sub_phase>=1)v.sub_phase-=1;
     v.mellow_phase+=f*detune;if(v.mellow_phase>=1)v.mellow_phase-=1;
     float a=plaits::Sine(v.sub_phase),b=plaits::Sine(v.mellow_phase);
     float tri=(f*5<.45f?plaits::Sine(v.sub_phase*5)*.04f:0)-(f*3<.45f?plaits::Sine(v.sub_phase*3)/9:0);
     out[k]=.38f*(a+b)+tone*.25f*tri+.08f*unit(t.harm)*plaits::Sine(v.sub_phase*2);
    }break;}
   case MD_BASS:
    for(unsigned k=0;k<n;k++){v.sub_phase+=f*.5f;if(v.sub_phase>=1)v.sub_phase-=1;
     out[k]=.65f*plaits::Sine(v.sub_phase*2)+.2f*unit(t.harm)*plaits::Sine(v.sub_phase)+.12f*tone*plaits::Sine(v.sub_phase*4);}
    break;
   case MD_CHOIR:{
    static const int chord[4][3]={{0,3,7},{0,4,7},{0,5,7},{0,7,12}};
    int ci=bound(t.harm*4/1001,0,3);
    for(int j=0;j<3;j++){
     int note=md_quantize((int)played_note+chord[ci][j],patch.root,patch.scale);
     float hz=440.0f*stmlib::SemitonesToRatio(note-69.0f+(j-1)*unit(t.shape)*.20f)/MD_RATE;
     v.choir[j].Render(hz,j?(.25f+.12f*tone):.45f,out,n);
    }break;}
   case MD_RINGS:
    v.rings.set_frequency(f);v.rings.set_structure(unit(t.harm)*.999f);v.rings.set_brightness(tone);
    v.rings.set_damping(unit(t.decay)*.9f);v.rings.set_position(.35f);v.rings.set_resolution(patch.quality?24:8);
    for(unsigned k=0;k<n;k++){exc[k]=noise()*v.excite*.65f*v.velocity;v.excite*=.985f;}
    v.rings.Process(exc,out,aux,n);break;
  }
  if(eco){memcpy(tmp,out,engine_n*sizeof(float));unsigned j=0;
   for(unsigned k=0;k<n;k++){if(!v.eco_phase){v.eco_previous=v.eco_next;v.eco_next=tmp[j++];out[k]=(v.eco_previous+v.eco_next)*.5f;}else out[k]=v.eco_next;v.eco_phase^=1;}
  }
  v.breath_phase+=n/(MD_RATE*(7.0f+i*2.3f));if(v.breath_phase>=1)v.breath_phase-=1;
  float cutoff=unit(t.cutoff)+tidal*.2f+unit(t.breathe)*.13f*plaits::Sine(v.breath_phase)+(smooth[2]-.5f)*.18f;
  if(t.filter==3)cutoff-=.55f*(1-v.lpg);
  cutoff=fminf(1.0f,fmaxf(0.0f,cutoff));
  v.cut_smooth+=(cutoff-v.cut_smooth)*slew40;v.res_smooth+=(unit(t.resonance)-v.res_smooth)*slew40;
  if(t.filter){float fc=cutoff_hz(v.cut_smooth)/MD_RATE;
   v.filter1.set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,filter_q(v.res_smooth));
   if(t.filter==2)v.filter2.set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,.707f);}
  float target=t.mute?0:unit(t.level)*.24f;
  float pan=unit(t.pan),dec=1.0f-1.0f/(MD_RATE*(.06f+unit(t.decay)*unit(t.decay)*16.0f));
  float attackstep=1.0f/(1+MD_RATE*8.0f*unit(t.attack)*unit(t.attack)),sendlevel=unit(t.send);
  const bool melodic=md_has_envelope(t.model);
  const float velocity=melodic?v.velocity:1,warp_target=unit(e.warps),ds=unit(e.delay_send),rs=unit(e.reverb_send),expr=expression[i];
  float track_peak=stats.track_peak[i];
  // Keep the exact envelope -> filter -> gain order, but let the SVF retain
  // coefficients and state in registers across a tile instead of the mixer.
  for(unsigned k=0;k<n;k++){
   float x=t.model==MD_ELEMENTS?(out[k]+aux[k]*.3f)*3.0f*v.velocity:t.model==MD_RINGS?(out[k]+aux[k])*12.0f:out[k];
   if(melodic){if(v.midi_gate<0){v.env*=.9975f;v.attacking=0;}else if(v.attacking){v.env+=attackstep;if(v.env>=1){v.env=1;v.attacking=0;}}else if(v.midi_gate==0)v.env*=dec;x*=v.env;}
   out[k]=x;
  }
  if(t.filter){v.filter1.Process<stmlib::FILTER_MODE_LOW_PASS>(out,out,n);if(t.filter==2)v.filter2.Process<stmlib::FILTER_MODE_LOW_PASS>(out,out,n);}
  // Local scalars prevent output-buffer aliasing from forcing repeated Voice
  // loads/stores. Same arithmetic and order; commit once after the tile.
  float mix_amp=v.amp;float mix_fade=v.fade;float mix_lpg=v.lpg;float mix_warp_smooth=v.warp_smooth;float mix_warp_phase=v.warp_phase;float mix_expression_smooth=v.expression_smooth;float mix_pan_smooth=v.pan_smooth;float mix_delay_send_smooth=v.delay_send_smooth;float mix_reverb_send_smooth=v.reverb_send_smooth;
  const bool duck_this_voice=(int)i!=patch.duck_source;
  for(unsigned k=0;k<n;k++) {
   mix_amp+=(target-mix_amp)*.004f;mix_fade+=(1-mix_fade)*.004f;
   float x=out[k];if(t.filter==3)x*=mix_lpg;
   mix_lpg*=dec;
   if(!isfinite(x)) {stats.nonfinite++;x=0;}
   // Individual oscillator outputs remain linear; tape and final mix retain peak protection.
   // Warps-inspired ring blending, not a claim to port the complete module.
   mix_warp_smooth+=(warp_target-mix_warp_smooth)*.002f;
   if(mix_warp_smooth>.000001f){mix_warp_phase+=f;if(mix_warp_phase>=1)mix_warp_phase-=1;x*=1-mix_warp_smooth*.5f+mix_warp_smooth*.5f*plaits::Sine(mix_warp_phase);}
   mix_expression_smooth+=(expr-mix_expression_smooth)*.003f;mix_pan_smooth+=(pan-mix_pan_smooth)*.003f;
   x=x*mix_amp*mix_fade*mix_expression_smooth*velocity;if(duck_this_voice)x*=duck[k];
   track_peak=fmaxf(track_peak,fabsf(x));
   lr[2*k]+=x*(1-mix_pan_smooth);lr[2*k+1]+=x*mix_pan_smooth;send[k]+=x*sendlevel;
   mix_delay_send_smooth+=(ds-mix_delay_send_smooth)*.002f;mix_reverb_send_smooth+=(rs-mix_reverb_send_smooth)*.002f;
   delay_send[k]+=x*mix_delay_send_smooth;room_send[k]+=x*mix_reverb_send_smooth;
  }
  v.amp=mix_amp;v.fade=mix_fade;v.lpg=mix_lpg;v.warp_smooth=mix_warp_smooth;v.warp_phase=mix_warp_phase;v.expression_smooth=mix_expression_smooth;v.pan_smooth=mix_pan_smooth;v.delay_send_smooth=mix_delay_send_smooth;v.reverb_send_smooth=mix_reverb_send_smooth;
  stats.track_peak[i]=track_peak;
  if(melodic&&!v.attacking&&v.env<.000001f)v.active=0;
 }
 const float echo_feedback=unit(patch.delay_fb),room_decay=.45f+.5f*unit(patch.reverb_time),effect_return=unit(patch.fx_level),echo_target=patch.delay_ms*44.1f;
 unsigned maxgrains=patch.quality?4:2;
 for(unsigned k=0;k<n;k++) {
  float wet[2]={};
  if(!grainclock){
   unsigned active=0;int slot=-1;
   for(unsigned j=0;j<4;j++){if(grains[j].life&&grains[j].age<grains[j].life)active++;else if(slot<0)slot=(int)j;}
   grainclock=32; // Defer rather than truncate a grain after a size/quality change.
   if(slot>=0&&active<maxgrains){
    unsigned life=2205+(unsigned)(unit(patch.grain_size)*11025);
    float travel=((patch.freeze?0.0f:1.0f)-grainrate)*life;
    float lo=260+fmaxf(0.0f,-travel),hi=(float)tape_valid-260-fmaxf(0.0f,travel);
    if(hi>lo){
     Grain &g=grains[slot];g.life=life;g.age=0;g.phase=0;g.inc=1.0f/life;
     float delay=lo+unit(patch.grain_pos)*(hi-lo);
     delay=fminf(hi,fmaxf(lo,delay+noise()*250));
     g.pos=(float)writehead-delay;while(g.pos<0)g.pos+=GN;
     g.pan=.5f+noise()*.4f;g.rate=grainrate;g.gain=2.0f/maxgrains;
     grainclock=(life+maxgrains-1)/maxgrains;
    }
   }
  }
  --grainclock;
  // Finish existing FULL grains when entering ECO; never silence a live slot.
  for(unsigned j=0;j<4;j++){Grain &g=grains[j];if(!g.life||g.age>=g.life)continue;
   float distance=(float)writehead-g.pos;if(distance<0)distance+=GN;
   float edge=fminf(1.0f,fmaxf(0.0f,fminf(distance,(float)tape_valid-distance))*(1.0f/256));
   float x=tape_read(g.pos),w=plaits::Sine(g.phase*.5f);w=w*w*g.gain*edge;
   wet[0]+=x*w*(1-g.pan);wet[1]+=x*w*g.pan;
   g.pos+=g.rate;if(g.pos>=GN)g.pos-=GN;if(g.pos<0)g.pos+=GN;
   g.age++;g.phase=g.age*g.inc;
  }
  float w=(wet[0]+wet[1])*.5f;feedback_lp+=(w-feedback_lp)*.65f;
  // Full-rate tape preserves the dry send's bandwidth; 131072 PCM16 samples.
  // Same 256 KiB memory footprint, now 2.97 s at 44.1 kHz rather than 5.94 s.
  if(!patch.freeze){float input=clip(send[k]+feedback_lp*fb);
   tape[writehead]=int16_t(input*32767);writehead=(writehead+1)&GM;if(tape_valid<GN)tape_valid++;stats.grain_writes++;
  }
  float effect[2]={};
  if(patch.fx_level)fx_active=true;
  if(fx_active){
  // Full-rate independent echo, interpolated time movement, bounded feedback.
  delay_smooth+=(echo_target-delay_smooth)*.00005f;
  float read=echo_head+16384.0f-delay_smooth;unsigned ri=(unsigned)read;float frac=read-ri;
  float delayed=(echo[ri&16383]*(1-frac)+echo[(ri+1)&16383]*frac)/32768.0f;
  echo_lp+=(delayed-echo_lp)*.25f;
  echo[echo_head]=int16_t(clip(delay_send[k]+echo_lp*echo_feedback)*32767);echo_head=(echo_head+1)&16383;
  float room_input=room_send[k];float hp=room_input-fx_highpass_in+.985f*fx_highpass_out;fx_highpass_in=room_input;fx_highpass_out=hp;
  static const unsigned lens[4]={887,1013,1151,1201};float rv[4],sum=0;
  for(unsigned j=0;j<4;j++){rv[j]=room[j][room_head[j]]/32768.0f;room_lp[j]+=(rv[j]-room_lp[j])*.3f;sum+=room_lp[j];}
  for(unsigned j=0;j<4;j++){float value=hp*.3f+(sum*.5f-room_lp[j])*room_decay;room[j][room_head[j]]=int16_t(clip(value)*32767);if(++room_head[j]>=lens[j])room_head[j]=0;}
  fx_smooth+=(effect_return-fx_smooth)*.002f;
  effect[0]=fx_smooth*(delayed*.5f+(rv[0]+rv[2])*.35f);effect[1]=fx_smooth*(delayed*.5f+(rv[1]+rv[3])*.35f);
  }
  for(unsigned c=0;c<2;c++) {
   float x=effect[c]+lr[2*k+c]+wet[c]*smooth[3]*1.7f*duck[k];
   float y=x-dcx[c]+.9997f*dcy[c];dcx[c]=x;dcy[c]=y;
   x=clip(y*1.9f);lr[2*k+c]=x;
   if(fabsf(x)>stats.peak)stats.peak=fabsf(x);
  }
 }
 stats.frames+=n;
}
}
extern "C" {
int md_hit(unsigned s,unsigned h,unsigned l,unsigned r) {if(!l||l>32)return 0;h=h>l?l:h;return (((s+r)%l)*h)%l<h;}
void md_init() {
 voices=reinterpret_cast<Voice*>(voices_raw);memset(voices_raw,0,sizeof voices_raw);memset(tape,0,sizeof tape);memset(grains,0,sizeof grains);
 memset(&stats,0,sizeof stats);memset(smooth,0,sizeof smooth);memset(dcx,0,sizeof dcx);memset(dcy,0,sizeof dcy);
 memset(midi_bank,0,sizeof midi_bank);qread=qwrite=writehead=grainclock=tape_valid=0;feedback_lp=0;rng=0x88888;stmlib::Random::Seed(rng);scene(0);playing=0;clock_source=midi_quantize=clock_pulse=0;clock_age=clock_last=0;clock_interval=0;
}
int md_clock_source(void){return clock_source;}
int md_midi_quantize(void){return midi_quantize;}
void md_midi(unsigned st,unsigned a,unsigned b,int port){
 a&=127;b&=127;
 if(st>=0xf8){
  if(!clock_source||port!=(clock_source==1?0:2))return;
  if(st==0xfa){step=0;seqrng=patch.seed;clock_pulse=0;playing=1;clock_age=0;clock_last=stats.frames;clock_interval=0;memset(repeats,0,sizeof repeats);}
  else if(st==0xfb){playing=1;clock_age=0;}
  else if(st==0xfc){playing=0;memset(repeats,0,sizeof repeats);}
  else if(st==0xf8&&playing){unsigned delta=stats.frames-clock_last;clock_last=stats.frames;
   if(delta>=MD_RATE*60/(220*24)&&delta<=MD_RATE*60/(30*24)){clock_interval=clock_interval?clock_interval*.85f+delta*.15f:delta;patch.tempo=bound((int)(MD_RATE*60/(24*clock_interval)+.5f),30,220);}
   clock_age=0;if(clock_pulse==0){untilstep=0;tick();}clock_pulse=(clock_pulse+1)%6;}
  return;
 }
 unsigned i=st&15;if(i==15&&(st&240)==0xb0&&a>=16&&a<=19){apply({MD_MACRO,(int)a-16,(int)b*1000/127});return;}if(i>=6)return;Voice &v=voices[i];unsigned type=st&240;
 if(type==0x90&&b){int note=midi_quantize?md_quantize(a,patch.root,patch.scale):bound(a,24,96);fire(i,note,b/127.0f);held[i]=a;v.midi_gate=md_has_envelope(v.model)?1:0;}
 else if(type==0x80||(type==0x90&&!b)){if(held[i]==(int)a){held[i]=-1;if(!sustain[i])v.midi_gate=-1;}}
 else if(type==0xc0){unsigned id=midi_bank[i]*128+a;if(id<MD_TONE_PRESETS)tone_preset(i,id);}
 else if(type==0xe0)bend[i]=((int)(a|(b<<7))-8192)*(2.0f/8192);
 else if(type==0xb0){int value=b*1000/127;
  if(a==0)midi_bank[i]=b;
  else if(a==32){} /* Only LSB zero supported; reserved, never a send. */
  else if(a==121){expression[i]=1;bend[i]=0;sustain[i]=0;if(held[i]<0)v.midi_gate=-1;}
  else if(a==64){sustain[i]=b>=64;if(!sustain[i]&&held[i]<0)v.midi_gate=-1;}
  else if(a==120)reset_voice(i);
  else if(a==123){held[i]=-1;sustain[i]=0;v.midi_gate=-1;}
  else if(a==11)expression[i]=b/127.0f;
  else {int c=a==7?MD_LEVEL:a==10?MD_PAN:a==74?MD_CUTOFF:a==71?MD_RESONANCE:a==91?MD_REVERB_SEND:a==12?MD_DELAY_SEND:a==13?MD_SEND:a==5?MD_GLIDE:-1;if(c>=0)apply({c,(int)i,value});}
 }
}
unsigned md_command_space(void){return 64-(qwrite-qread);}
int md_command(int c,int t,int v) {if(c<0||c>=MD_COMMAND_COUNT)return 0;unsigned w=qwrite;if(w-qread>=64){stats.dropped_commands++;return 0;}queue[w&63]={c,t,v};fence();qwrite=w+1;return 1;}
void md_render(float *lr,unsigned n) {
 while(n){while(qread!=qwrite){fence();Command e=queue[qread&63];apply(e);fence();qread++;}
  unsigned z=n<Q?n:Q;
  if(playing){
   if(!clock_source&&untilstep<=0)tick();
   for(unsigned i=0;i<6;i++)if(repeats[i].remaining&&repeats[i].left<=0){
    if(!patch.track[i].mute)fire(i,repeats[i].note,repeats[i].velocity);
    repeats[i].remaining--;repeats[i].left+=repeats[i].spacing;
   }
   unsigned to=(!clock_source&&untilstep>0)?(unsigned)ceilf(untilstep):0;if(to&&z>to)z=to;
   for(unsigned i=0;i<6;i++)if(repeats[i].remaining){to=(unsigned)ceilf(repeats[i].left);if(to&&z>to)z=to;}
   untilstep-=z;for(unsigned i=0;i<6;i++)if(repeats[i].remaining)repeats[i].left-=z;
  }
  if(clock_source&&playing){clock_age+=z;if(clock_age>MD_RATE*2){playing=0;memset(repeats,0,sizeof repeats);}}
  quantum(lr,z);lr+=z*2;n-=z;
 }
}
const md_patch *md_state(){return &patch;}const md_stats *md_status(){return &stats;}int md_playing(){return playing;}
int md_validate(const md_patch *p) {
 if(!p||p->magic!=0x4D445231||p->version!=6||!p->seed||p->tempo<30||p->tempo>220)return 0;
 if(p->biome_id< -1||p->biome_id>=MD_BIOMES||p->edited_mask>127)return 0;
 for(int i=0;i<6;i++)if(p->tone_preset[i]< -1||p->tone_preset[i]>=MD_TONE_PRESETS)return 0;
 for(int j=0;j<4;j++)if(p->macro[j]<0||p->macro[j]>1000)return 0;
 if(p->locked<0||p->locked>1||p->freeze<0||p->freeze>1||p->quality<0||p->quality>1)return 0;
 if(p->grain_size<0||p->grain_size>1000||p->grain_pos<0||p->grain_pos>1000||p->feedback<0||p->feedback>850)return 0;
 if(p->root<0||p->root>11||p->scale<0||p->scale>7||p->transpose< -24||p->transpose>24||p->swing<0||p->swing>600||p->grain_pitch< -24||p->grain_pitch>24||p->reverse<0||p->reverse>1||p->duck_source< -1||p->duck_source>5||p->duck_amount<0||p->duck_amount>1000||p->duck_release<0||p->duck_release>1000)return 0;
 for(int i=0;i<6;i++){const md_track &t=p->track[i];if(t.attack<0||t.attack>1000||t.ratchet<1||t.ratchet>4||t.accent<0||t.accent>1000)return 0;if(t.model<0||t.model>=MD_MODELS||t.length<1||t.length>32||t.hits<0||t.hits>t.length||t.rotate<0||t.rotate>=t.length||t.note<24||t.note>96||t.mute<0||t.mute>1||t.speed<1||t.speed>8)return 0;
 if(t.filter<0||t.filter>3)return 0;const int q[]={t.tone,t.harm,t.decay,t.level,t.send,t.shape,t.cutoff,t.resonance,t.breathe};for(int j=0;j<9;j++)if(q[j]<0||q[j]>1000)return 0;if(t.prob<0||t.prob>1000||t.pan<0||t.pan>1000)return 0;}
 for(int i=0;i<6;i++){const md_extension &e=p->ext[i];const int values[]={e.delay_send,e.reverb_send,e.glide,e.marbles,e.tides,e.tides_rate,e.warps};for(int v:values)if(v<0||v>1000)return 0;if(e.bass_mode<0||e.bass_mode>2||e.pattern<0||e.pattern>3||e.edit_step<0||e.edit_step>15)return 0;for(int v:e.notes)if(v< -1||v>24)return 0;}
 if(p->delay_ms<40||p->delay_ms>350||p->delay_fb<0||p->delay_fb>800||p->reverb_time<0||p->reverb_time>850||p->fx_level<0||p->fx_level>1000)return 0;
 return 1;
}
int md_load(const md_patch *p){if(playing||!md_validate(p))return 0;patch=*p;previous=patch;seqrng=patch.seed;clear_audio();for(int j=0;j<4;j++)smooth[j]=unit(patch.macro[j]);for(int i=0;i<6;i++)reset_voice(i);return 1;}
unsigned md_memory_bytes(){return sizeof element_body+sizeof element_strike+sizeof echo+sizeof room+sizeof voices_raw+sizeof tape+sizeof grains+sizeof patch+sizeof previous+sizeof stats+sizeof queue+sizeof repeats+512;}
int md_decode(md_patch *out,const void *data,unsigned bytes){
 if(!out||!data)return 0;
 if(bytes==sizeof(md_patch)){md_patch p;memcpy(&p,data,sizeof p);if(p.version==5)p.version=6;if(!md_validate(&p))return 0;*out=p;return 1;}
 struct OldTrack{int32_t values[14];};
 struct OldPatch{uint32_t magic,version,seed;int32_t macro[4],tempo,locked,freeze,grain_size,grain_pos,feedback,quality;OldTrack track[6];};
 struct V2Track{int32_t values[17];};
 struct V2Patch{uint32_t magic,version,seed;int32_t macro[4],tempo,locked,freeze,grain_size,grain_pos,feedback,quality;V2Track track[6];int32_t globals[9];};
 md_patch p={};
 if(bytes==652){memcpy(&p,data,652);if(p.version!=4)return 0;extension_defaults(p);if(!md_validate(&p))return 0;*out=p;return 1;}
 if(bytes==620){
  memcpy(&p,data,620);if(p.version!=3)return 0;
  extension_defaults(p);p.biome_id=-1;p.edited_mask=1;for(int i=0;i<6;i++)p.tone_preset[i]=-1;
  if(!md_validate(&p))return 0;*out=p;return 1;
 }
 if(bytes==sizeof(V2Patch)){
  V2Patch old;memcpy(&old,data,sizeof old);if(old.version!=2)return 0;
  memcpy(&p,&old,offsetof(V2Patch,track));memcpy(&p.root,old.globals,sizeof old.globals);
  for(int i=0;i<6;i++)memcpy(&p.track[i],&old.track[i],sizeof(V2Track));
 }else if(bytes==sizeof(OldPatch)){
  OldPatch old;memcpy(&old,data,sizeof old);if(old.version!=1)return 0;
  memcpy(&p,&old,offsetof(OldPatch,track));p.scale=1;p.duck_source=-1;p.duck_release=350;
  for(int i=0;i<6;i++){memcpy(&p.track[i],&old.track[i],sizeof(OldTrack));p.track[i].ratchet=1;}
 }else return 0;
 extension_defaults(p);p.biome_id=-1;p.edited_mask=1;for(int i=0;i<6;i++)p.tone_preset[i]=-1;
 for(int i=0;i<6;i++){p.track[i].shape=p.track[i].decay;p.track[i].cutoff=850;}
 if(!md_validate(&p))return 0;*out=p;return 1;
}
int md_import(const void *data,unsigned bytes){
 if(playing)return 0;md_patch decoded;return md_decode(&decoded,data,bytes)&&md_load(&decoded);
}
const char *md_biome_name(int id){return id<0?"USER BIOME":biome_bank[bound(id,0,MD_BIOMES-1)].name;}
const char *md_biome_family(int id){return id<0?"CUSTOM":biome_families[biome_bank[bound(id,0,MD_BIOMES-1)].family];}
const char *md_tone_name(int id){return id<0?"BIOME VOICE":tone_bank[bound(id,0,MD_TONE_PRESETS-1)].name;}
int md_tone_model(int id){return tone_bank[bound(id,0,MD_TONE_PRESETS-1)].model;}
int md_has_envelope(int model){return model==MD_FM||model==MD_VA||model==MD_HARMONIC||model==MD_CHOIR||model==MD_BASS||model==MD_MELLOW||model==MD_SATIN;}
int md_cutoff_hz(int value){return (int)cutoff_hz(unit(value));}

int md_quantize(int note,int root,int scale){
 static const unsigned masks[]={0xfff,0x5ad,0xab5,0x6ad,0x4a9,0x295,0x555,0x5ab};
 note=bound(note,24,96);root=bound(root,0,11);unsigned mask=masks[bound(scale,0,7)];
 for(int delta=0;delta<12;delta++){
  int low=note-delta,high=note+delta;
  if(low>=24&&(mask&(1u<<((low-root+12)%12))))return low;
  if(high<=96&&(mask&(1u<<((high-root+12)%12))))return high;
 }return note;
}
const char *md_scale_name(int s){static const char *names[]={"CHROMATIC","MINOR","MAJOR","DORIAN","MINOR PENT","MAJOR PENT","WHOLE TONE","PHRYGIAN"};return names[bound(s,0,7)];}
const char *md_model_name(int m){static const char *names[]={"PLAITS KICK","PLAITS SNARE","PLAITS HAT","PLAITS FM","PLAITS VA","RINGS MODAL","PLAITS HARMONIC","SINE CHOIR","ROUND BASS","ELEMENTS STRIKE","MELLOW DRIFT","SATIN MORPH"};return names[bound(m,0,MD_MODELS-1)];}
}
