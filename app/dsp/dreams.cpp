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
#include "rings/dsp/fx/chorus.h"
#ifndef PLAT_HOST
#define POOL __attribute__((section(".pool")))
#else
#define POOL
#endif
namespace {
#include "satin_tables.inc"
#include "scale_table.inc"
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
 float orn_from,orn_age; int orn_style; // Monades gamaka: start offset, seconds since the note, style
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
static stmlib::Svf send_hp_filter[2];
// Monades: Rings chorus on the dry mix (4 KiB line), and Dattorro / Griesinger input diffusion before the room
// (four allpasses, 1210 samples). Both are skipped entirely at 0.
static uint16_t chorus_line[2048] POOL;static rings::Chorus chorus;
static int16_t diffuser[1210];static unsigned diffuser_head[4];static const unsigned diffuser_len[4]={150,214,319,527}; // Monades: 12 dB high-pass on the echo and room sends
static void reset_fx(){send_hp_filter[0].Init();send_hp_filter[1].Init();memset(chorus_line,0,sizeof chorus_line);chorus.Init(chorus_line);memset(diffuser,0,sizeof diffuser);memset(diffuser_head,0,sizeof diffuser_head);fx_active=false;memset(echo,0,sizeof echo);memset(room,0,sizeof room);memset(room_head,0,sizeof room_head);memset(room_lp,0,sizeof room_lp);echo_head=0;echo_lp=fx_smooth=fx_highpass_in=fx_highpass_out=0;delay_smooth=8820;}
static void v10_defaults(md_patch &p){p.version=10;p.motion_every=0;}
static void v9_defaults(md_patch &p){for(int i=0;i<6;i++){p.gamaka[i]=MD_GAMAKA_OFF;p.gamaka_amount[i]=500;}p.vadi=p.samvadi=-1;v10_defaults(p);}
static void v8_defaults(md_patch &p){for(int i=0;i<6;i++)p.gen[i]={MD_GEN_OFF,500,8,12};p.form=0;p.form_bars=1;p.form_drops=2;v9_defaults(p);}
static void v7_defaults(md_patch &p){p.send_hp=0;p.echo_tone=p.room_tone=500;p.echo_sync=p.room_diffusion=p.chorus=0;p.chorus_depth=500;v8_defaults(p);for(int i=0;i<6;i++)for(int s=0;s<2;s++)p.lfo[i][s]={3,MD_LFO_SINE,s?MD_TO_PAN:MD_TO_CUTOFF,0};}
static void extension_defaults(md_patch &p){v7_defaults(p);p.delay_ms=200;p.delay_fb=300;p.reverb_time=450;p.fx_level=0;memset(p.ext,0,sizeof p.ext);for(int i=0;i<6;i++){p.ext[i].tides_rate=250;for(int j=0;j<16;j++)p.ext[i].notes[j]=(j%4==0?0:j%4==2?7:-1);}}

struct Grain { float pos,phase,inc,pan,rate,gain; unsigned life,age; };
struct Repeat { float left,spacing,note,velocity; unsigned remaining; };
static Repeat repeats[6];
static Grain grains[4];
static md_patch patch,previous;
static md_stats stats;
static unsigned writehead,grainclock,tape_valid;
static uint8_t tape_peak[256];static unsigned tape_peak_col;
// Display-only scope per voice: 128 points after the filter, stride chosen so about two periods fit 64 points.
static int16_t scope_buf[6][128];static unsigned scope_pos[6],scope_phase[6]; // display-only overview of the tape, 512 samples a column
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
static uint16_t custom_scale=0x091; /* CUSTOM: root, major third, fifth until the player edits it */
static int last_generated[6]; /* per track, for ascent / descent sets */
static int motion_held[6]; /* Motion's degree, held for motion_every steps */
// Ascending into a note uses the ascent set, descending the descent set (ragas such as Desh or Jaunpuri, classical melodic minor).
static unsigned micro_mask(const MicroDef &m){unsigned mask=1;for(unsigned k=0;k<m.count;k++)mask|=1u<<((int)(m.degree[k]/100.0f+.5f)%12);return mask;} /* nearest semitones, for twelve-tone views */
static unsigned scale_mask_for(int scale,int down){scale=bound(scale,0,SCALE_COUNT-1);if(scale==SCALE_CUSTOM)return custom_scale|1u;if(scale>=SCALE_TWELVE)return micro_mask(micro_defs[scale-SCALE_TWELVE]);return down?scale_defs[scale].down:scale_defs[scale].up;}
// Microtonal: the nearest degree in cents (ties go down), repeated every period above the root; a fractional MIDI note.
static float quantize_micro(float note,int root,const MicroDef &m){
 note=fminf(96.0f,fmaxf(24.0f,note));root=bound(root,0,11);
 float rel=(note-root)*100.0f,k=floorf(rel/m.period),r=rel-k*m.period,best=0,dist=r;
 for(unsigned j=0;j<=m.count;j++){float d=j<m.count?m.degree[j]:m.period,e=fabsf(r-d);if(e<dist){dist=e;best=d;}}
 float out=root+(k*m.period+best)*.01f;
 while(out>96.0f)out-=m.period*.01f;while(out<24.0f)out+=m.period*.01f;
 return out;
}
static int quantize_mask(int note,int root,unsigned mask){
 note=bound(note,24,96);root=bound(root,0,11);
 for(int delta=0;delta<12;delta++){
  int low=note-delta,high=note+delta;
  if(low>=24&&(mask&(1u<<((low-root+12)%12))))return low;
  if(high<=96&&(mask&(1u<<((high-root+12)%12))))return high;
 }return note;
}
static float quantize_any(float note,int root,int scale,int down){
 scale=bound(scale,0,SCALE_COUNT-1);
 if(scale>=SCALE_TWELVE)return quantize_micro(note,root,micro_defs[scale-SCALE_TWELVE]);
 return (float)quantize_mask((int)floorf(note+.5f),root,scale_mask_for(scale,down));
}
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
 reset_fx();tape_valid=0;writehead=grainclock=0;memset(tape_peak,0,sizeof tape_peak);tape_peak_col=0;
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
#include "midi_out.inc"
#include "lfo.inc"
static float clamp01(float x){return x<0?0:x>1?1:x;}
#include "generative.inc"
// Echo sync: the division in sixteenths.
static const float echo_sync_len[MD_ECHO_SYNCS]={0,.5f,2/3.0f,1,4/3.0f,1.5f,2,8/3.0f,3,4}; // sixteenths
static float echo_ms_now(){
 if(!patch.echo_sync)return (float)patch.delay_ms;
 float ms=echo_sync_len[bound(patch.echo_sync,1,MD_ECHO_SYNCS-1)]*15000.0f/patch.tempo;
 while(ms>350)ms*=.5f;while(ms<40)ms*=2; // the line holds 371 ms: a slow tempo plays half the division (still on the grid)
 return ms;
}
// Echo / room damping: 500 keeps Mutable Dreams' exact coefficient.
static float tone_coef(int v,float lo,float mid,float hi){return v==500?mid:v<500?lo+(mid-lo)*v*.002f:mid+(hi-mid)*(v-500)*.002f;}
static void fire(unsigned i,float note,float velocity) {
 Voice &v=voices[i];
 {int style=patch.gamaka[i];v.orn_style=0;v.orn_age=0;
  if(style&&patch.track[i].model>=MD_FM){
   if(style==MD_GAMAKA_MIX){unsigned r=patch.seed^(stats.events[i]*0x9e3779b9u)^(i*0x85ebca6bu);random_word(r);style=MD_GAMAKA_KAN+(int)(r%3);}
   float prev=stats.last_pitch[i]>0?stats.last_pitch[i]:note;
   v.orn_from=style==MD_GAMAKA_KAN?scale_neighbour(note,1)-note:style==MD_GAMAKA_ANDOLAN?scale_neighbour(note,-1)-note:fmaxf(-7.0f,fminf(7.0f,prev-note));
   v.orn_style=style;}
  stats.gamaka_style[i]=v.orn_style;}
 v.trigger=1;v.midi_gate=0;v.target_note=note;if(!patch.ext[i].glide||!v.active)v.note=note;v.velocity=velocity;v.active=1;
 stats.events[i]++;stats.last_note[i]=(int)note;stats.last_pitch[i]=note;
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
 case MD_PLAY:{int was=playing;playing=!!v;memset(repeats,0,sizeof repeats);if(playing){step=0;untilstep=0;seqrng=patch.seed;beat=0;beat_limit=1;beat_wraps=0;gen_reset();memset(motion_held,0,sizeof motion_held);}
  if(routing.clock&&!clock_source&&playing!=was){out_push(playing?0xfa:0xfc,0,0);clock_left=0;}if(!playing)out_all_off();}break;
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
 case MD_TRIGGER:fire(i,t.model>=MD_FM?quantize_any((float)(v+patch.transpose),patch.root,patch.scale,0):(float)bound(v,24,96),.8f);break;
 case MD_ATTACK:t.attack=bound(v,0,1000);break;
 case MD_RATCHET:t.ratchet=bound(v,1,4);break;
 case MD_SHAPE:t.shape=bound(v,0,1000);break;
 case MD_CUTOFF:t.cutoff=bound(v,0,1000);break;
 case MD_RESONANCE:t.resonance=bound(v,0,1000);break;
 case MD_FILTER:if(t.filter!=bound(v,0,MD_FILTER_MODES-1)){v=bound(v,0,MD_FILTER_MODES-1);t.filter=v;voices[i].filter1.Reset();voices[i].filter2.Reset();voices[i].fade=0;}break;
 case MD_BREATHE:t.breathe=bound(v,0,1000);break;
 case MD_ACCENT:t.accent=bound(v,0,1000);break;
 case MD_SWING:patch.swing=bound(v,0,600);break;
 case MD_ROOT:patch.root=bound(v,0,11);break;
 case MD_SCALE:patch.scale=bound(v,0,SCALE_COUNT-1);break;
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
 case MD_LFO_RATE:case MD_LFO_SHAPE:case MD_LFO_TARGET:case MD_LFO_DEPTH:{md_lfo &l=patch.lfo[bound(e.t,0,11)%6][bound(e.t,0,11)/6];
  if(e.c==MD_LFO_RATE)l.rate=bound(v,0,MD_LFO_RATES-1);else if(e.c==MD_LFO_SHAPE)l.shape=bound(v,0,MD_LFO_SHAPES-1);else if(e.c==MD_LFO_TARGET)l.target=bound(v,0,MD_LFO_TARGETS-1);else l.depth=bound(v,-1000,1000);break;}
 case MD_SEND_HP:patch.send_hp=bound(v,0,1000);break;
 case MD_ECHO_TONE:patch.echo_tone=bound(v,0,1000);break;
 case MD_ROOM_TONE:patch.room_tone=bound(v,0,1000);break;
 case MD_ECHO_SYNC:patch.echo_sync=bound(v,0,MD_ECHO_SYNCS-1);break;
 case MD_ROOM_DIFFUSION:patch.room_diffusion=bound(v,0,1000);break;
 case MD_CHORUS:patch.chorus=bound(v,0,1000);break;
 case MD_CHORUS_DEPTH:patch.chorus_depth=bound(v,0,1000);break;
 case MD_GEN_SOURCE:patch.gen[i].source=bound(v,0,MD_GEN_SOURCES-1);break;
 case MD_GEN_DEJAVU:patch.gen[i].dejavu=bound(v,0,1000);break;
 case MD_GEN_LOOP:patch.gen[i].loop=bound(v,1,16);break;
 case MD_GEN_SPREAD:patch.gen[i].spread=bound(v,0,24);break;
 case MD_FORM:patch.form=bound(v,0,1000);break;
 case MD_FORM_BARS:patch.form_bars=bound(v,0,3);break;
 case MD_FORM_DROPS:patch.form_drops=bound(v,0,3);break;
 case MD_GAMAKA:patch.gamaka[i]=bound(v,0,MD_GAMAKAS-1);break;
 case MD_GAMAKA_AMOUNT:patch.gamaka_amount[i]=bound(v,0,1000);break;
 case MD_VADI:patch.vadi=bound(v,-1,11);break;
 case MD_MOTION_EVERY:patch.motion_every=bound(v,0,4);break;
 case MD_SAMVADI:patch.samvadi=bound(v,-1,11);break;
 case MD_PANIC:if(playing&&routing.clock&&!clock_source)out_push(0xfc,0,0);playing=0;out_all_off();clear_audio();for(unsigned j=0;j<6;j++)reset_voice(j);break;
 }
}
static void tick() {
 float duration=MD_RATE*60.0f/(patch.tempo*4.0f)*(1+(step%2?-1:1)*unit(patch.swing)*.5f);
 if((step%32)==0) {if(patch.locked)seqrng=patch.seed;else patch.seed=random_word(seqrng)|1;}
 static const int degree[7]={0,2,3,5,7,10,12};
 float tension=.6f;unsigned dropped=0;
 if(patch.form){unsigned bars=form_bar_len[bound(patch.form_bars,0,3)],bar=step/16,sec=(bar/bars)%4,cyc=bar/(bars*4);
  tension=form_tension_of[sec];dropped=form_mask(cyc,sec);
  stats.form_section=sec;stats.form_cycle=cyc;stats.form_drop_mask=dropped;stats.form_pos=(float)(step%(16*bars))/(16*bars);}
 for(unsigned i=0;i<6;i++) {
  const md_track &t=patch.track[i];if(step%(unsigned)t.speed)continue;
  unsigned local=step/t.speed;
  int hits=bound(t.hits+(patch.macro[0]-500)*t.length/1500+(int)floorf(lfo_mod[i][MD_TO_HITS]*t.length+.5f)+(patch.form?(int)floorf((tension-.6f)*unit(patch.form)*t.length*.8f+.5f):0),0,t.length);
  unsigned roll=random_word(seqrng);
  const bool hit=patch.ext[i].bass_mode==2||md_hit(local,hits,t.length,t.rotate),rest=(dropped>>i)&1;
  bool pass=roll%1000<(unsigned)t.prob;int gen_off=0;const bool gen=patch.gen[i].source&&hit&&!t.mute&&!rest;
  if(gen){GenState &g=gens[i];const unsigned len=(unsigned)bound(patch.gen[i].loop,1,16),pos=g.count%len;
   const bool replay=((g.valid>>pos)&1)&&random_word(g.rng)%1000<(unsigned)patch.gen[i].dejavu;
   if(replay){gen_off=g.off[pos];pass=(g.gate>>pos)&1;}
   else{gen_off=gen_draw(i,g);g.off[pos]=gen_off;g.valid|=1u<<pos;g.gate=(g.gate&~(1u<<pos))|((unsigned)pass<<pos);}
   g.prev=gen_off;g.count++;
   stats.gen_valid[i]=g.valid;stats.gen_gate[i]=g.gate;stats.gen_pos[i]=pos;stats.gen_fresh[i]=!replay;memcpy(stats.gen_off[i],g.off,sizeof g.off);}
  {const uint32_t bit=1u<<(local%(unsigned)t.length);
   stats.step_played[i]&=~bit;stats.step_skipped[i]&=~bit;if(hit&&!t.mute&&!rest){if(pass)stats.step_played[i]|=bit;else stats.step_skipped[i]|=bit;}}
  if(hit&&pass&&!t.mute&&!rest) {
   int note=t.note;float pitch;
   const md_extension &e=patch.ext[i];
   if(e.bass_mode){int off=e.notes[local%(e.bass_mode==2?(t.length>16?16:t.length):16)];if(off<0)continue;note+=off;
    // Marbles-inspired remembered variation: deterministic for each seed/position,
    // independent of audio noise and stable across repeated phrases when locked.
    unsigned mr=patch.seed^(i*0x9e3779b9u)^((local%16+1)*0x85ebca6bu);unsigned choice=random_word(mr);
    if((local%4)!=0&&choice%1000<(unsigned)e.marbles){static const int interval[]={0,3,5,7};note=t.note+interval[(choice>>12)&3];}
   }
   if(t.model>=MD_FM) {
    if(gen&&!e.bass_mode)note+=gen_off;
    else if(!e.bass_mode&&patch.motion_every){const unsigned every=1u<<bound(patch.motion_every,0,4); // a new variation on the beat, held in between
     if(local%every==0)motion_held[i]=(roll>>10)%1000<(unsigned)patch.macro[1]?degree[(roll>>20)%7]:0;note+=motion_held[i];}
    else if(!e.bass_mode&&(roll>>10)%1000<(unsigned)patch.macro[1])note+=degree[(roll>>20)%7];
    {int target=note+patch.transpose+(int)floorf(lfo_mod[i][MD_TO_NOTES]*12+.5f); /* LFO steps stay in the scale */
     // Pivot notes: a generated note within two semitones of the vadi (or samvadi) is drawn to it, a third of the time.
     if(patch.vadi>=0||patch.samvadi>=0){const int pivots[2]={patch.vadi,patch.samvadi};unsigned pr=roll^0x5bd1e995u;random_word(pr);
      if(pr%3==0)for(int k=0;k<2;k++){if(pivots[k]<0)continue;int pc=(patch.root+pivots[k])%12,d=((pc-target)%12+18)%12-6;if(d>=-2&&d<=2&&d){target+=d;break;}}}pitch=quantize_any((float)target,patch.root,patch.scale,target<last_generated[i]);note=(int)floorf(pitch+.5f);last_generated[i]=note;}
   }
   float velocity=.8f+unit(t.accent)*((local%t.length)==0?.2f:-.25f);
   if(t.model>=MD_FM&&(patch.vadi>=0||patch.samvadi>=0)){int pc=((note-patch.root)%12+12)%12;if(pc==patch.vadi)velocity=fminf(1.0f,velocity+.15f);else if(pc==patch.samvadi)velocity=fminf(1.0f,velocity+.08f);} // pivots sing out
   if(t.model<MD_FM)pitch=(float)note;
   fire(i,pitch,velocity);out_note(i,pitch,velocity,duration*.5f/t.ratchet);
   repeats[i]={(duration/t.ratchet),duration/t.ratchet,pitch,velocity,(unsigned)t.ratchet-1};
  }
 }
 beat_wraps=step/384;beat=(float)(step%384);beat_limit=beat+1;
 stats.step=step++;untilstep+=duration;
}
static void quantum(float *lr,unsigned n) {
 // Conservative admission guard from 0.4.1 FM-1 costs with full-rate tape, LP24 and maximum repeats.
 // Count unmuted slots before playback so the tape cannot prefill four FULL grains
 // immediately before a dense six-voice attack. Existing audible tails also count.
 if(patch.quality){static const unsigned weight[MD_MODELS]={8,13,8,18,10,26,17,8,8,30,10,12};unsigned cost=12;
  // Provisional additional weights, pending physical-device measurement.
  if(fx_active||patch.fx_level)cost+=10;
  if(patch.chorus)cost+=3;if(patch.send_hp||patch.room_diffusion)cost+=2; // Monades estimates, pending FM-1 measurement
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
 {const float meter_decay=n>32?.984064f:.992f;for(unsigned j=0;j<3;j++)stats.fx_peak[j]*=meter_decay;stats.out_peak[0]*=meter_decay;stats.out_peak[1]*=meter_decay;stats.limit_peak*=meter_decay;}
 for(unsigned j=0;j<4;j++)smooth[j]+=(unit(patch.macro[j])-smooth[j])*slew25;
 lfo_update(n);
 float delay_send[Q]={},room_send[Q]={};
 float send[Q]={},out[Q],aux[Q],tmp[Q],exc[Q],duck[Q];memset(lr,0,n*2*sizeof(float));
 float duckdec=1.0f-1.0f/(MD_RATE*(.04f+unit(patch.duck_release)*1.46f));
 for(unsigned k=0;k<n;k++){duck[k]=1-duckenv*unit(patch.duck_amount)*.9f;duckenv*=duckdec;}
 stats.duck_gain=n?duck[n-1]:1;
 float grainrate=stmlib::SemitonesToRatio((float)patch.grain_pitch)*(patch.reverse?-1:1);
 float fb=unit(patch.feedback);
 for(unsigned i=0;i<6;i++) {
  md_track &t=patch.track[i];Voice &v=voices[i];bool trig=v.trigger;v.trigger=0;
  if(trig) {v.active=1;v.attacking=t.attack>0; if(!v.attacking)v.env=1;v.excite=1;v.lpg=1;}
  /* Only FM/VA have an observable final envelope. Never cut resonator/drum tails. */
  if(!v.active){stats.skipped_quanta++;continue;}stats.voice_quanta++;
  const md_extension &e=patch.ext[i];const float *lm=lfo_mod[i];
  // LFO targets; with depth 0 every value below is exactly the knob's.
  const float harm=lm[MD_TO_HARMONICS]?clamp01(unit(t.harm)+lm[MD_TO_HARMONICS]*.5f):unit(t.harm),shp=lm[MD_TO_SHAPE]?clamp01(unit(t.shape)+lm[MD_TO_SHAPE]*.5f):unit(t.shape);
  const float decay_u=lm[MD_TO_DECAY]?clamp01(unit(t.decay)+lm[MD_TO_DECAY]*.5f):unit(t.decay),att=lm[MD_TO_ATTACK]?clamp01(unit(t.attack)+lm[MD_TO_ATTACK]*.5f):unit(t.attack);
  v.note+=(v.target_note-v.note)*(e.glide?n/(float)(n+MD_RATE*.35f*unit(e.glide)):1.0f);
  v.tide_phase+=n/(MD_RATE*(.5f+unit(e.tides_rate)*19.5f));if(v.tide_phase>=1)v.tide_phase-=1;
  stats.tide_phase[i]=v.tide_phase;
  float tidal=(1.0f-4.0f*fabsf(v.tide_phase-.5f))*unit(e.tides);
  float tone=fminf(.98f,fmaxf(t.model==MD_HARMONIC?0.0f:.02f,unit(t.tone)+(smooth[2]-.5f)*.65f+lm[MD_TO_TONE]*.5f));
  bend_smooth[i]+=(bend[i]-bend_smooth[i])*slew40;
  float ornament=0;if(v.orn_style){ornament=gamaka_shape(v.orn_style,patch.gamaka_amount[i],v.orn_from,v.orn_age*1000);v.orn_age+=n*(1.0f/MD_RATE);}
  stats.gamaka_now[i]=ornament;stats.gamaka_age[i]=v.orn_age*1000;
  float played_note=v.note+bend_smooth[i]+lm[MD_TO_PITCH]*12.0f+ornament;
  float f=440.0f*stmlib::SemitonesToRatio(played_note-69.0f)/MD_RATE;
  plaits::EngineParameters p={trig?1:0,played_note,tone,shp,harm*.999f,v.velocity};bool done=false;
  memset(out,0,sizeof out);memset(aux,0,sizeof aux);
  bool eco=patch.quality==0&&(t.model==MD_FM||t.model==MD_VA||t.model==MD_HARMONIC);
  unsigned engine_n=eco?(n+(v.eco_phase?0:1))/2:n;
  if(eco)p.note+=12.0f; // same concert pitch at 22.05 kHz; linear reconstruction below.
  switch(t.model) {
   case MD_KICK:v.kick.Render(false,trig,v.velocity,f,tone,decay_u,harm,.2f,out,n);break;
   case MD_SNARE:v.snare.Render(false,trig,v.velocity,f,tone,decay_u,harm,out,n);break;
   case MD_HAT:v.hat.Render(false,trig,v.velocity,f,tone,decay_u,harm,tmp,exc,out,n);break;
   case MD_FM:if(engine_n)v.fm.Render(p,out,nullptr,engine_n,&done);break;
   case MD_VA:if(engine_n)v.va.Render(p,out,nullptr,engine_n,&done);break;
   case MD_HARMONIC:if(engine_n)v.additive.Render(p,out,nullptr,engine_n,&done);break;
   case MD_ELEMENTS:{
    elements::Exciter &strike=element_strike[i];elements::Resonator &body=element_body[i];
    if(trig)v.strike_gate=MD_RATE*(.03f+.5f*shp);
    unsigned strike_flags=(trig?elements::EXCITER_FLAG_RISING_EDGE:0)|(v.strike_gate>0?elements::EXCITER_FLAG_GATE:0);v.strike_gate=fmaxf(0,v.strike_gate-n);
    strike.set_model(t.shape<334?elements::EXCITER_MODEL_MALLET:t.shape<667?elements::EXCITER_MODEL_PLECTRUM:elements::EXCITER_MODEL_PARTICLES);
    // In the Plectrum region, keep its main pulse 1.7-5.2 ms after the primer.
    // The original 14-38 ms mapping was continually restarted by 220 BPM
    // four-ratchet rolls (17 ms spacing), leaving only the tiny primer audible.
    strike.set_parameter(t.shape>=334&&t.shape<667?.05f+.15f*(t.shape-334)/332.0f:.1f+.8f*shp);
    strike.set_timbre(fminf(.95f,tone));strike.Process(strike_flags,exc,n);
    body.set_frequency(f);body.set_geometry(fminf(.999f,harm));body.set_brightness(tone);body.set_damping(decay_u*.8f);body.set_position(.3f);body.set_resolution(element_modes);
    memset(tmp,0,sizeof tmp);body.Process(tmp,exc,out,aux,n);break;}
   case MD_SATIN:{
    // Original rounded wavetable oscillator. Reduce harmonics continuously
    // before Nyquist; interpolate phase and waveform, with no oversampling RAM.
    v.satin_morph+=(harm*2.999f-v.satin_morph)*slew35;
    v.satin_tone+=(tone-v.satin_tone)*slew35;
    int shape=(int)v.satin_morph;float morph=v.satin_morph-shape;
    v.mellow_drift+=n/(MD_RATE*(9.0f+i));if(v.mellow_drift>=1)v.mellow_drift-=1;
    float f2=f*stmlib::SemitonesToRatio(.13f*shp*plaits::Sine(v.mellow_drift));
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
    float detune=stmlib::SemitonesToRatio(.10f*shp*plaits::Sine(v.mellow_drift));
    for(unsigned k=0;k<n;k++){
     v.sub_phase+=f;if(v.sub_phase>=1)v.sub_phase-=1;
     v.mellow_phase+=f*detune;if(v.mellow_phase>=1)v.mellow_phase-=1;
     float a=plaits::Sine(v.sub_phase),b=plaits::Sine(v.mellow_phase);
     float tri=(f*5<.45f?plaits::Sine(v.sub_phase*5)*.04f:0)-(f*3<.45f?plaits::Sine(v.sub_phase*3)/9:0);
     out[k]=.38f*(a+b)+tone*.25f*tri+.08f*harm*plaits::Sine(v.sub_phase*2);
    }break;}
   case MD_BASS:
    for(unsigned k=0;k<n;k++){v.sub_phase+=f*.5f;if(v.sub_phase>=1)v.sub_phase-=1;
     out[k]=.65f*plaits::Sine(v.sub_phase*2)+.2f*harm*plaits::Sine(v.sub_phase)+.12f*tone*plaits::Sine(v.sub_phase*4);}
    break;
   case MD_CHOIR:{
    static const int chord[4][3]={{0,3,7},{0,4,7},{0,5,7},{0,7,12}};
    int ci=bound(t.harm*4/1001,0,3);
    for(int j=0;j<3;j++){
     int note=md_quantize((int)played_note+chord[ci][j],patch.root,patch.scale);
     float hz=440.0f*stmlib::SemitonesToRatio(note-69.0f+(j-1)*shp*.20f)/MD_RATE;
     v.choir[j].Render(hz,j?(.25f+.12f*tone):.45f,out,n);
    }break;}
   case MD_RINGS:
    v.rings.set_frequency(f);v.rings.set_structure(harm*.999f);v.rings.set_brightness(tone);
    v.rings.set_damping(decay_u*.9f);v.rings.set_position(.35f);v.rings.set_resolution(patch.quality?24:8);
    for(unsigned k=0;k<n;k++){exc[k]=noise()*v.excite*.65f*v.velocity;v.excite*=.985f;}
    v.rings.Process(exc,out,aux,n);break;
  }
  if(eco){memcpy(tmp,out,engine_n*sizeof(float));unsigned j=0;
   for(unsigned k=0;k<n;k++){if(!v.eco_phase){v.eco_previous=v.eco_next;v.eco_next=tmp[j++];out[k]=(v.eco_previous+v.eco_next)*.5f;}else out[k]=v.eco_next;v.eco_phase^=1;}
  }
  v.breath_phase+=n/(MD_RATE*(7.0f+i*2.3f));if(v.breath_phase>=1)v.breath_phase-=1;
  float cutoff=unit(t.cutoff)+tidal*.2f+unit(t.breathe)*.13f*plaits::Sine(v.breath_phase)+(smooth[2]-.5f)*.18f+lm[MD_TO_CUTOFF]*.5f;
  if(t.filter==3)cutoff-=.55f*(1-v.lpg);
  cutoff=fminf(1.0f,fmaxf(0.0f,cutoff));
  v.cut_smooth+=(cutoff-v.cut_smooth)*slew40;v.res_smooth+=(clamp01(unit(t.resonance)+lm[MD_TO_RESONANCE]*.5f)-v.res_smooth)*slew40;
  stats.cutoff_now[i]=cutoff_hz(v.cut_smooth);stats.q_now[i]=filter_q(v.res_smooth);
  if(t.filter){float fc=cutoff_hz(v.cut_smooth)/MD_RATE;
   v.filter1.set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,filter_q(v.res_smooth));
   if(t.filter==2||t.filter==7)v.filter2.set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,.707f);}
  float target=t.mute?0:unit(t.level)*.24f*(1+lm[MD_TO_LEVEL]);
  float pan=clamp01(unit(t.pan)+lm[MD_TO_PAN]*.5f),dec=1.0f-1.0f/(MD_RATE*(.06f+decay_u*decay_u*16.0f));
  float attackstep=1.0f/(1+MD_RATE*8.0f*att*att),sendlevel=clamp01(unit(t.send)+lm[MD_TO_GRAIN_SEND]*.5f);
  const bool melodic=md_has_envelope(t.model);
  const float velocity=melodic?v.velocity:1,warp_target=unit(e.warps),ds=clamp01(unit(e.delay_send)+lm[MD_TO_ECHO_SEND]*.5f),rs=clamp01(unit(e.reverb_send)+lm[MD_TO_ROOM_SEND]*.5f),expr=expression[i];
  float track_peak=stats.track_peak[i];
  // Keep the exact envelope -> filter -> gain order, but let the SVF retain
  // coefficients and state in registers across a tile instead of the mixer.
  for(unsigned k=0;k<n;k++){
   float x=t.model==MD_ELEMENTS?(out[k]+aux[k]*.3f)*3.0f*v.velocity:t.model==MD_RINGS?(out[k]+aux[k])*12.0f:out[k];
   if(melodic){if(v.midi_gate<0){v.env*=.9975f;v.attacking=0;}else if(v.attacking){v.env+=attackstep;if(v.env>=1){v.env=1;v.attacking=0;}}else if(v.midi_gate==0)v.env*=dec;x*=v.env;}
   out[k]=x;
  }
  if(t.filter==4||t.filter==7){v.filter1.Process<stmlib::FILTER_MODE_HIGH_PASS>(out,out,n);if(t.filter==7)v.filter2.Process<stmlib::FILTER_MODE_HIGH_PASS>(out,out,n);}
  else if(t.filter==5)v.filter1.Process<stmlib::FILTER_MODE_BAND_PASS_NORMALIZED>(out,out,n);
  else if(t.filter==6){v.filter1.Process<stmlib::FILTER_MODE_BAND_PASS_NORMALIZED>(out,tmp,n);for(unsigned k=0;k<n;k++)out[k]-=tmp[k];} // notch = input - band
  else if(t.filter){v.filter1.Process<stmlib::FILTER_MODE_LOW_PASS>(out,out,n);if(t.filter==2)v.filter2.Process<stmlib::FILTER_MODE_LOW_PASS>(out,out,n);}
  {unsigned stride=(unsigned)(1.0f/(f*32.0f));stride=stride<1?1:stride>32?32:stride;unsigned ph=scope_phase[i],ps=scope_pos[i];
   for(unsigned k=0;k<n;k++)if(++ph>=stride){ph=0;float y=out[k]*8192.0f;scope_buf[i][ps]=(int16_t)(y>32767?32767:y<-32767?-32767:y);ps=(ps+1)&127;}
   scope_phase[i]=ph;scope_pos[i]=ps;}
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
  stats.env_now[i]=melodic?v.env:0;stats.env_attacking[i]=melodic&&v.attacking;
  if(melodic&&!v.attacking&&v.env<.000001f)v.active=0;
 }
 const float echo_feedback=lfo_echo_fb?fminf(.8f,clamp01(unit(patch.delay_fb)+lfo_echo_fb*.5f)):unit(patch.delay_fb),room_decay=.45f+.5f*unit(patch.reverb_time),effect_return=unit(patch.fx_level),echo_target=fminf(15435.0f,fmaxf(1764.0f,(patch.echo_sync?echo_ms_now():(float)patch.delay_ms)*44.1f*(1+lfo_echo_time*.5f)));
 const float echo_glide=lfo_echo_time?.002f:.00005f,echo_tone=tone_coef(patch.echo_tone,.04f,.25f,.9f),room_tone=tone_coef(patch.room_tone,.05f,.3f,.85f);
 const float diffusion=patch.room_diffusion*.0007f; // up to 0.7, Dattorro uses 0.625-0.75
 const bool send_hp=patch.send_hp>0;if(send_hp){float fc=md_send_hp_hz(patch.send_hp)/(float)MD_RATE;send_hp_filter[0].set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,.707f);send_hp_filter[1].set_f_q<stmlib::FREQUENCY_ACCURATE>(fc,.707f);}
 if(patch.chorus){ // de-interleave in place through the scratch buffers: no extra memory
  for(unsigned k=0;k<n;k++){out[k]=lr[2*k];aux[k]=lr[2*k+1];}
  chorus.set_amount(patch.chorus*.001f);chorus.set_depth(patch.chorus_depth*.001f);chorus.Process(out,aux,n);
  for(unsigned k=0;k<n;k++){lr[2*k]=out[k];lr[2*k+1]=aux[k];}
 }
 unsigned maxgrains=patch.quality?4:2;
 float fx_peak[3]={stats.fx_peak[0],stats.fx_peak[1],stats.fx_peak[2]},out_peak[2]={stats.out_peak[0],stats.out_peak[1]},limit_peak=stats.limit_peak;
 for(unsigned k=0;k<n;k++) {
  float wet[2]={};
  if(!grainclock){
   unsigned active=0;int slot=-1;
   for(unsigned j=0;j<4;j++){if(grains[j].life&&grains[j].age<grains[j].life)active++;else if(slot<0)slot=(int)j;}
   grainclock=32; // Defer rather than truncate a grain after a size/quality change.
   if(slot>=0&&active<maxgrains){
    unsigned life=2205+(unsigned)(clamp01(unit(patch.grain_size)+lfo_grain_size*.5f)*11025);
    float travel=((patch.freeze?0.0f:1.0f)-grainrate)*life;
    float lo=260+fmaxf(0.0f,-travel),hi=(float)tape_valid-260-fmaxf(0.0f,travel);
    if(hi>lo){
     Grain &g=grains[slot];g.life=life;g.age=0;g.phase=0;g.inc=1.0f/life;
     float delay=lo+clamp01(unit(patch.grain_pos)+lfo_grain_pos*.5f)*(hi-lo);
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
   tape[writehead]=int16_t(input*32767);
   {unsigned col=writehead>>9;uint8_t a=(uint8_t)(fabsf(input)*254.9f);if(col!=tape_peak_col){tape_peak_col=col;tape_peak[col]=0;}if(a>tape_peak[col])tape_peak[col]=a;}
   writehead=(writehead+1)&GM;if(tape_valid<GN)tape_valid++;stats.grain_writes++;
  }
  float effect[2]={};
  if(patch.fx_level)fx_active=true;
  if(fx_active){
  // Full-rate independent echo, interpolated time movement, bounded feedback.
  delay_smooth+=(echo_target-delay_smooth)*echo_glide;
  float read=echo_head+16384.0f-delay_smooth;unsigned ri=(unsigned)read;float frac=read-ri;
  float delayed=(echo[ri&16383]*(1-frac)+echo[(ri+1)&16383]*frac)/32768.0f;
  echo_lp+=(delayed-echo_lp)*echo_tone;
  float echo_in=delay_send[k],room_input=room_send[k];
  if(send_hp){echo_in=send_hp_filter[0].Process<stmlib::FILTER_MODE_HIGH_PASS>(echo_in);room_input=send_hp_filter[1].Process<stmlib::FILTER_MODE_HIGH_PASS>(room_input);}
  echo[echo_head]=int16_t(clip(echo_in+echo_lp*echo_feedback)*32767);echo_head=(echo_head+1)&16383;
  float hp=room_input-fx_highpass_in+.985f*fx_highpass_out;fx_highpass_in=room_input;fx_highpass_out=hp;
  if(diffusion>0){float x=hp;unsigned base=0;for(unsigned j=0;j<4;j++){int16_t &cell=diffuser[base+diffuser_head[j]];float d=cell*(1.0f/32768.0f),y=d-diffusion*x;cell=int16_t(clip(x+diffusion*y)*32767);if(++diffuser_head[j]>=diffuser_len[j])diffuser_head[j]=0;base+=diffuser_len[j];x=y;}hp=x;} // smears the room's attack
  static const unsigned lens[4]={887,1013,1151,1201};float rv[4],sum=0;
  for(unsigned j=0;j<4;j++){rv[j]=room[j][room_head[j]]/32768.0f;room_lp[j]+=(rv[j]-room_lp[j])*room_tone;sum+=room_lp[j];}
  for(unsigned j=0;j<4;j++){float value=hp*.3f+(sum*.5f-room_lp[j])*room_decay;room[j][room_head[j]]=int16_t(clip(value)*32767);if(++room_head[j]>=lens[j])room_head[j]=0;}
  fx_smooth+=(effect_return-fx_smooth)*.002f;
  effect[0]=fx_smooth*(delayed*.5f+(rv[0]+rv[2])*.35f);effect[1]=fx_smooth*(delayed*.5f+(rv[1]+rv[3])*.35f);
  // Meters only: the returns as they reach the final 1.9x mix stage.
  fx_peak[1]=fmaxf(fx_peak[1],fabsf(fx_smooth*delayed)*.95f);fx_peak[2]=fmaxf(fx_peak[2],fx_smooth*fmaxf(fabsf(rv[0]+rv[2]),fabsf(rv[1]+rv[3]))*.665f);
  }
  fx_peak[0]=fmaxf(fx_peak[0],fmaxf(fabsf(wet[0]),fabsf(wet[1]))*smooth[3]*3.23f*duck[k]);
  for(unsigned c=0;c<2;c++) {
   float x=effect[c]+lr[2*k+c]+wet[c]*smooth[3]*1.7f*duck[k];
   float y=x-dcx[c]+.9997f*dcy[c];dcx[c]=x;dcy[c]=y;
   limit_peak=fmaxf(limit_peak,fabsf(y*1.9f));
   x=clip(y*1.9f);lr[2*k+c]=x;
   if(fabsf(x)>stats.peak)stats.peak=fabsf(x);
   out_peak[c]=fmaxf(out_peak[c],fabsf(x));
  }
 }
 for(unsigned j=0;j<3;j++)stats.fx_peak[j]=fx_peak[j];
 stats.out_peak[0]=out_peak[0];stats.out_peak[1]=out_peak[1];stats.limit_peak=limit_peak;
 stats.frames+=n;
}
}
extern "C" {
int md_hit(unsigned s,unsigned h,unsigned l,unsigned r) {if(!l||l>32)return 0;h=h>l?l:h;return (((s+r)%l)*h)%l<h;}
void md_init() {
 voices=reinterpret_cast<Voice*>(voices_raw);memset(voices_raw,0,sizeof voices_raw);memset(tape,0,sizeof tape);memset(grains,0,sizeof grains);
 memset(&stats,0,sizeof stats);memset(smooth,0,sizeof smooth);memset(dcx,0,sizeof dcx);memset(dcy,0,sizeof dcy);
 out_r=out_w=owed_n=0;memset(&routing,0,sizeof routing);clock_left=0;memset(chan_bend,0,sizeof chan_bend);
 for(int i=0;i<6;i++)last_generated[i]=60;
 memset(midi_bank,0,sizeof midi_bank);qread=qwrite=writehead=grainclock=tape_valid=0;feedback_lp=0;rng=0x88888;stmlib::Random::Seed(rng);scene(0);playing=0;clock_source=midi_quantize=clock_pulse=0;clock_age=clock_last=0;clock_interval=0;
}
void md_set_midi_routing(const md_midi_routing *r){
 // Turning output off (or changing the routing) first releases every note still owed on the old routing.
 out_all_off();for(unsigned ch=0;ch<16;ch++)out_bend(ch,0);routing=*r;routing.mode=bound(routing.mode,0,2);routing.clock=!!routing.clock;
 for(unsigned z=0;z<8;z++){routing.zone[z].lo=bound(routing.zone[z].lo,0,127);routing.zone[z].hi=bound(routing.zone[z].hi,0,127);routing.zone[z].ch=bound(routing.zone[z].ch,0,16);}
}
int md_midi_take(uint8_t m[3]){unsigned r=out_r;if(r==out_w)return 0;fence();const OutMsg &o=outq[r&255];m[0]=o.st;m[1]=o.d1;m[2]=o.d2;fence();out_r=r+1;return 1;}
unsigned md_scope(int track,float *dst){
 // Oldest first; the caller aligns on a rising zero crossing.
 track=bound(track,0,5);unsigned ps=scope_pos[track];
 for(unsigned k=0;k<128;k++)dst[k]=scope_buf[track][(ps+k)&127]*(1.0f/8192);
 return 128;
}
void md_tape_view(md_tape *v){
 // Same placement rule as new grains in quantum(), so the screen shows where the next ones start.
 memcpy(v->peak,tape_peak,sizeof v->peak);v->head=writehead;v->valid=tape_valid;v->length=GN;v->freeze=patch.freeze;
 float rate=stmlib::SemitonesToRatio((float)patch.grain_pitch)*(patch.reverse?-1:1);
 float life=2205+(unsigned)(unit(patch.grain_size)*11025),travel=((patch.freeze?0.0f:1.0f)-rate)*life;
 float lo=260+fmaxf(0.0f,-travel),hi=(float)tape_valid-260-fmaxf(0.0f,travel);
 v->zone=hi>lo?lo+unit(patch.grain_pos)*(hi-lo):-1;v->zone_len=life;
 for(unsigned j=0;j<4;j++){const Grain &g=grains[j];int on=g.life&&g.age<g.life;v->grain[j].active=on;if(!on)continue;
  float d=(float)writehead-g.pos;if(d<0)d+=GN;float w=plaits::Sine(g.phase*.5f);
  v->grain[j].behind=d;v->grain[j].len=(float)g.life;v->grain[j].env=w*w;}
}
int md_clock_source(void){return clock_source;}
int md_midi_quantize(void){return midi_quantize;}
void md_midi(unsigned st,unsigned a,unsigned b,int port){
 a&=127;b&=127;
 if(st>=0xf8){
  if(!clock_source||port!=(clock_source==1?0:2))return;
  if(st==0xfa){step=0;seqrng=patch.seed;clock_pulse=0;playing=1;beat=0;beat_limit=1;beat_wraps=0;gen_reset();clock_age=0;clock_last=stats.frames;clock_interval=0;memset(repeats,0,sizeof repeats);}
  else if(st==0xfb){playing=1;clock_age=0;}
  else if(st==0xfc){playing=0;memset(repeats,0,sizeof repeats);out_all_off();}
  else if(st==0xf8&&playing){unsigned delta=stats.frames-clock_last;clock_last=stats.frames;
   if(delta>=MD_RATE*60/(220*24)&&delta<=MD_RATE*60/(30*24)){clock_interval=clock_interval?clock_interval*.85f+delta*.15f:delta;patch.tempo=bound((int)(MD_RATE*60/(24*clock_interval)+.5f),30,220);}
   clock_age=0;if(clock_pulse==0){untilstep=0;tick();}clock_pulse=(clock_pulse+1)%6;}
  return;
 }
 unsigned i=st&15;if(i==15&&(st&240)==0xb0&&a>=16&&a<=19){apply({MD_MACRO,(int)a-16,(int)b*1000/127});return;}if(i>=6)return;Voice &v=voices[i];unsigned type=st&240;
 if(type==0x90&&b){float note=midi_quantize?quantize_any((float)a,patch.root,patch.scale,0):(float)bound(a,24,96);fire(i,note,b/127.0f);held[i]=a;v.midi_gate=md_has_envelope(v.model)?1:0;}
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
    if(!patch.track[i].mute){fire(i,repeats[i].note,repeats[i].velocity);out_note(i,repeats[i].note,repeats[i].velocity,repeats[i].spacing*.5f);}
    repeats[i].remaining--;repeats[i].left+=repeats[i].spacing;
   }
   unsigned to=(!clock_source&&untilstep>0)?(unsigned)ceilf(untilstep):0;if(to&&z>to)z=to;
   for(unsigned i=0;i<6;i++)if(repeats[i].remaining){to=(unsigned)ceilf(repeats[i].left);if(to&&z>to)z=to;}
   if(routing.clock&&!clock_source){if(clock_left<=0){out_push(0xf8,0,0);clock_left+=MD_RATE*60.0f/(patch.tempo*24.0f);}to=(unsigned)ceilf(clock_left);if(to&&z>to)z=to;clock_left-=z;}
   untilstep-=z;for(unsigned i=0;i<6;i++)if(repeats[i].remaining)repeats[i].left-=z;
  }
  out_advance(z);
  if(clock_source&&playing){clock_age+=z;if(clock_age>MD_RATE*2){playing=0;memset(repeats,0,sizeof repeats);out_all_off();}}
  quantum(lr,z);lr+=z*2;n-=z;
 }
}
const md_patch *md_state(){return &patch;}const md_stats *md_status(){return &stats;}int md_playing(){return playing;}
int md_validate(const md_patch *p) {
 if(!p||p->magic!=0x4D445231||p->version!=10||!p->seed||p->tempo<30||p->tempo>220)return 0;
 if(p->biome_id< -1||p->biome_id>=MD_BIOMES||p->edited_mask>127)return 0;
 for(int i=0;i<6;i++)if(p->tone_preset[i]< -1||p->tone_preset[i]>=MD_TONE_PRESETS)return 0;
 for(int j=0;j<4;j++)if(p->macro[j]<0||p->macro[j]>1000)return 0;
 if(p->locked<0||p->locked>1||p->freeze<0||p->freeze>1||p->quality<0||p->quality>1)return 0;
 if(p->grain_size<0||p->grain_size>1000||p->grain_pos<0||p->grain_pos>1000||p->feedback<0||p->feedback>850)return 0;
 if(p->root<0||p->root>11||p->scale<0||p->scale>=SCALE_COUNT||p->transpose< -24||p->transpose>24||p->swing<0||p->swing>600||p->grain_pitch< -24||p->grain_pitch>24||p->reverse<0||p->reverse>1||p->duck_source< -1||p->duck_source>5||p->duck_amount<0||p->duck_amount>1000||p->duck_release<0||p->duck_release>1000)return 0;
 for(int i=0;i<6;i++){const md_track &t=p->track[i];if(t.attack<0||t.attack>1000||t.ratchet<1||t.ratchet>4||t.accent<0||t.accent>1000)return 0;if(t.model<0||t.model>=MD_MODELS||t.length<1||t.length>32||t.hits<0||t.hits>t.length||t.rotate<0||t.rotate>=t.length||t.note<24||t.note>96||t.mute<0||t.mute>1||t.speed<1||t.speed>8)return 0;
 if(t.filter<0||t.filter>=MD_FILTER_MODES)return 0;const int q[]={t.tone,t.harm,t.decay,t.level,t.send,t.shape,t.cutoff,t.resonance,t.breathe};for(int j=0;j<9;j++)if(q[j]<0||q[j]>1000)return 0;if(t.prob<0||t.prob>1000||t.pan<0||t.pan>1000)return 0;}
 for(int i=0;i<6;i++){const md_extension &e=p->ext[i];const int values[]={e.delay_send,e.reverb_send,e.glide,e.marbles,e.tides,e.tides_rate,e.warps};for(int v:values)if(v<0||v>1000)return 0;if(e.bass_mode<0||e.bass_mode>2||e.pattern<0||e.pattern>3||e.edit_step<0||e.edit_step>15)return 0;for(int v:e.notes)if(v< -1||v>24)return 0;}
 if(p->delay_ms<40||p->delay_ms>350||p->delay_fb<0||p->delay_fb>800||p->reverb_time<0||p->reverb_time>850||p->fx_level<0||p->fx_level>1000)return 0;
 if(p->send_hp<0||p->send_hp>1000||p->echo_tone<0||p->echo_tone>1000||p->room_tone<0||p->room_tone>1000)return 0;
 for(int i=0;i<6;i++){const md_gen &g=p->gen[i];if(g.source<0||g.source>=MD_GEN_SOURCES||g.dejavu<0||g.dejavu>1000||g.loop<1||g.loop>16||g.spread<0||g.spread>24)return 0;}
 for(int i=0;i<6;i++)if(p->gamaka[i]<0||p->gamaka[i]>=MD_GAMAKAS||p->gamaka_amount[i]<0||p->gamaka_amount[i]>1000)return 0;
 if(p->motion_every<0||p->motion_every>4)return 0;
 if(p->vadi< -1||p->vadi>11||p->samvadi< -1||p->samvadi>11)return 0;
 if(p->form<0||p->form>1000||p->form_bars<0||p->form_bars>3||p->form_drops<0||p->form_drops>3)return 0;
 if(p->echo_sync<0||p->echo_sync>=MD_ECHO_SYNCS||p->room_diffusion<0||p->room_diffusion>1000||p->chorus<0||p->chorus>1000||p->chorus_depth<0||p->chorus_depth>1000)return 0;
 for(int i=0;i<6;i++)for(int s=0;s<2;s++){const md_lfo &l=p->lfo[i][s];if(l.rate<0||l.rate>=MD_LFO_RATES||l.shape<0||l.shape>=MD_LFO_SHAPES||l.target<0||l.target>=MD_LFO_TARGETS||l.depth< -1000||l.depth>1000)return 0;}
 return 1;
}
int md_load(const md_patch *p){if(playing||!md_validate(p))return 0;patch=*p;previous=patch;seqrng=patch.seed;clear_audio();for(int j=0;j<4;j++)smooth[j]=unit(patch.macro[j]);for(int i=0;i<6;i++)reset_voice(i);return 1;}
unsigned md_memory_bytes(){return sizeof chorus_line+sizeof chorus+sizeof diffuser+sizeof element_body+sizeof element_strike+sizeof echo+sizeof room+sizeof voices_raw+sizeof tape+sizeof grains+sizeof patch+sizeof previous+sizeof stats+sizeof queue+sizeof repeats+512;}
int md_decode(md_patch *out,const void *data,unsigned bytes){
 if(!out||!data)return 0;
 if(bytes==sizeof(md_patch)){md_patch p;memcpy(&p,data,sizeof p);if(!md_validate(&p))return 0;*out=p;return 1;}
 // Monades v9 (1676 bytes): Motion varies every step.
 if(bytes==offsetof(md_patch,motion_every)){md_patch p;memcpy(&p,data,bytes);if(p.version!=9)return 0;v10_defaults(p);if(!md_validate(&p))return 0;*out=p;return 1;}
 // Monades v8 (1620 bytes): no gamakas, no pivot notes.
 if(bytes==offsetof(md_patch,gamaka)){md_patch p;memcpy(&p,data,bytes);if(p.version!=8)return 0;v9_defaults(p);if(!md_validate(&p))return 0;*out=p;return 1;}
 // Monades v7 (1512 bytes): generators and form start off.
 if(bytes==offsetof(md_patch,gen)){md_patch p;memcpy(&p,data,bytes);if(p.version!=7)return 0;v8_defaults(p);if(!md_validate(&p))return 0;*out=p;return 1;}
 // Mutable Dreams 1.1 (v5 / v6, 1292 bytes): everything Monades added starts neutral.
 if(bytes==offsetof(md_patch,send_hp)){md_patch p;memcpy(&p,data,bytes);if(p.version!=5&&p.version!=6)return 0;v7_defaults(p);if(!md_validate(&p))return 0;*out=p;return 1;}
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

unsigned md_scale_mask(int scale){return scale_mask_for(scale,0);}
unsigned md_scale_mask_down(int scale){return scale_mask_for(scale,1);}
int md_quantize(int note,int root,int scale){return quantize_mask(note,root,scale_mask_for(scale,0));}
const char *md_scale_name(int s){s=bound(s,0,SCALE_COUNT-1);return s>=SCALE_TWELVE?micro_defs[s-SCALE_TWELVE].name:scale_defs[s].name;}
const char *md_scale_family(int s){s=bound(s,0,SCALE_COUNT-1);return scale_families[s>=SCALE_TWELVE?micro_defs[s-SCALE_TWELVE].family:scale_defs[s].family];}
float md_quantize_pitch(float note,int root,int scale){return quantize_any(note,root,scale,0);}
int md_scale_cents(int s,float *degrees,int max,float *period){
 // Degrees in cents above the root (the root first); twelve-tone scales give their ascent set.
 s=bound(s,0,SCALE_COUNT-1);int n=0;
 if(s>=SCALE_TWELVE){const MicroDef &m=micro_defs[s-SCALE_TWELVE];*period=m.period;if(n<max)degrees[n++]=0;for(unsigned k=0;k<m.count&&n<max;k++)degrees[n++]=m.degree[k];return n;}
 unsigned mask=scale_mask_for(s,0);*period=1200;for(int k=0;k<12&&n<max;k++)if(mask&(1u<<k))degrees[n++]=k*100.0f;return n;
}
int md_scale_count(void){return SCALE_COUNT;}
int md_scale_custom(void){return SCALE_CUSTOM;}
int md_scale_at(int position){return scale_order[bound(position,0,SCALE_COUNT-1)];}
int md_scale_position(int scale){for(int k=0;k<SCALE_COUNT;k++)if(scale_order[k]==scale)return k;return 0;}
float md_lfo_wave(int track,int slot,uint32_t cycle,float phase){track=bound(track,0,5);slot=bound(slot,0,1);return lfo_shape(patch.lfo[track][slot].shape,track,slot,cycle,phase-floorf(phase));}
float md_lfo_sixteenths(int rate){return rate>=0&&rate<MD_LFO_SYNCED?lfo_sync[rate]:0;}
float md_lfo_hz(int rate,int tempo){rate=bound(rate,0,MD_LFO_RATES-1);return rate<MD_LFO_SYNCED?tempo/15.0f/lfo_sync[rate]:lfo_free_hz(rate);}
const char *md_lfo_rate_name(int rate){rate=bound(rate,0,MD_LFO_RATES-1);return rate<MD_LFO_SYNCED?lfo_sync_names[rate]:"FREE";}
const char *md_lfo_shape_name(int s){static const char *const n[MD_LFO_SHAPES]={"SINE","TRIANGLE","RAMP UP","RAMP DOWN","SQUARE","SAMPLE & HOLD","SMOOTH RANDOM"};return n[bound(s,0,MD_LFO_SHAPES-1)];}
const char *md_lfo_target_name(int t){static const char *const n[MD_LFO_TARGETS]={"CUTOFF","RESONANCE","LEVEL","PAN","PITCH","TONE","HARMONICS","SHAPE","ECHO SEND","ROOM SEND","GRAIN SEND","ECHO TIME","GRAIN POS","ATTACK","DECAY","ECHO FEEDBACK","GRAIN SIZE","NOTES IN SCALE","HITS"};return n[bound(t,0,MD_LFO_TARGETS-1)];}
float md_echo_ms(void){return echo_ms_now();}
unsigned md_form_mask(unsigned c,unsigned s){return form_mask(c,s);}
float md_form_tension(unsigned s){return form_tension_of[s&3];}
int md_form_bars(void){return (int)form_bar_len[bound(patch.form_bars,0,3)];}
const char *md_gamaka_name(int s){static const char *const n[MD_GAMAKAS]={"OFF","KAN","MEEND","ANDOLAN","MIX"};return n[bound(s,0,MD_GAMAKAS-1)];}
float md_gamaka_shape(int style,int amount,float from,float ms){return gamaka_shape(style,amount,from,ms);}
const char *md_gen_source_name(int s){static const char *const n[MD_GEN_SOURCES]={"OFF","RANDOM","MARKOV","MOTIF"};return n[bound(s,0,MD_GEN_SOURCES-1)];}
float md_echo_sync_len16(int s){return echo_sync_len[bound(s,0,MD_ECHO_SYNCS-1)];}
const char *md_echo_sync_name(int s){static const char *const n[MD_ECHO_SYNCS]={"OFF","1/32","1/16 T","1/16","1/8 T","1/16 .","1/8","1/4 T","1/8 .","1/4"};return n[bound(s,0,MD_ECHO_SYNCS-1)];}
int md_send_hp_hz(int v){return v>0?(int)(20.0f*stmlib::SemitonesToRatio(bound(v,0,1000)*.0797262f)+.5f):0;}
void md_set_custom_scale(unsigned mask){custom_scale=(uint16_t)((mask&0xfff)|1u);}
const char *md_model_name(int m){static const char *names[]={"PLAITS KICK","PLAITS SNARE","PLAITS HAT","PLAITS FM","PLAITS VA","RINGS MODAL","PLAITS HARMONIC","SINE CHOIR","ROUND BASS","ELEMENTS STRIKE","MELLOW DRIFT","SATIN MORPH"};return names[bound(m,0,MD_MODELS-1)];}
}
