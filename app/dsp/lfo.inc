/* SPDX-License-Identifier: MIT OR GPL-3.0-only
 * Monades: two LFOs per track, synced to the sequencer or free. Included by dreams.cpp, inside its anonymous namespace. */
// Monades LFOs. Synced ones read the sequencer's position in sixteenths, so they restart with PLAY and follow
// an external MIDI clock; free ones keep their own phase. Random shapes come from the seed, cycle by cycle.
static const float lfo_sync[MD_LFO_SYNCED]={128,64,32,16,8,6,16/3.0f,4,3,8/3.0f,2,4/3.0f,1,2/3.0f,.5f};
static const char *const lfo_sync_names[MD_LFO_SYNCED]={"8 BARS","4 BARS","2 BARS","1 BAR","1/2","1/4 .","1/2 T","1/4","1/8 .","1/4 T","1/8","1/8 T","1/16","1/16 T","1/32"};
static float beat,beat_limit;static uint32_t beat_wraps; // sixteenths, wrapped every 384 (a multiple of every synced cycle)
static float lfo_free_phase[6][2];static uint32_t lfo_free_cycle[6][2];
static float lfo_free_hz(int rate){return .02f*stmlib::SemitonesToRatio((rate-MD_LFO_SYNCED)*119.589f/15.0f);} // 0.02..20 Hz
static float lfo_random(unsigned track,unsigned slot,uint32_t cycle){unsigned r=patch.seed^(track*0x9e3779b9u)^(slot*0x7f4a7c15u)^(cycle*0x85ebca6bu);random_word(r);random_word(r);return (r>>8)*(1.0f/8388608.0f)-1.0f;}
static float lfo_shape(int shape,unsigned track,unsigned slot,uint32_t cycle,float ph){
 switch(shape){
  case MD_LFO_TRIANGLE:return ph<.25f?4*ph:ph<.75f?2-4*ph:4*ph-4;
  case MD_LFO_RAMP_UP:return 2*ph-1;
  case MD_LFO_RAMP_DOWN:return 1-2*ph;
  case MD_LFO_SQUARE:return ph<.5f?1.0f:-1.0f;
  case MD_LFO_SAMPLE_HOLD:return lfo_random(track,slot,cycle);
  case MD_LFO_SMOOTH_RANDOM:{float a=lfo_random(track,slot,cycle),b=lfo_random(track,slot,cycle+1);return a+(b-a)*(.5f-.5f*plaits::Sine(ph*.5f+.25f));}
  default:return plaits::Sine(ph);
 }
}
// Modulation for this quantum: per track and target, plus the two shared targets.
static float lfo_mod[6][MD_LFO_TARGETS],lfo_echo_time,lfo_grain_pos,lfo_echo_fb,lfo_grain_size;
static void lfo_update(unsigned n){
 beat+=n*patch.tempo*4.0f/(60.0f*MD_RATE);if(playing&&beat>beat_limit)beat=beat_limit; // never run past the next step
 if(beat>=384){beat-=384;beat_limit-=384;beat_wraps++;}
 memset(lfo_mod,0,sizeof lfo_mod);lfo_echo_time=lfo_grain_pos=lfo_echo_fb=lfo_grain_size=0;
 for(unsigned i=0;i<6;i++)for(unsigned s=0;s<2;s++){const md_lfo &l=patch.lfo[i][s];float ph;uint32_t cycle;
  if(l.rate<MD_LFO_SYNCED){float len=lfo_sync[l.rate],pos=beat/len;uint32_t whole=(uint32_t)pos;ph=pos-whole;cycle=beat_wraps*(uint32_t)(384/len+.5f)+whole;}
  else{float &f=lfo_free_phase[i][s];f+=n*lfo_free_hz(l.rate)/MD_RATE;while(f>=1){f-=1;lfo_free_cycle[i][s]++;}ph=f;cycle=lfo_free_cycle[i][s];}
  float v=lfo_shape(l.shape,i,s,cycle,ph);stats.lfo_phase[i][s]=ph;stats.lfo_cycle[i][s]=cycle;stats.lfo_value[i][s]=v;
  if(!l.depth)continue;
  float m=v*l.depth*.001f;
  if(l.target==MD_TO_LEVEL)m=-fabsf(l.depth*.001f)*(1-(l.depth<0?-v:v))*.5f; // tremolo: 0 at the top, -depth at the bottom
  if(l.target==MD_TO_ECHO_TIME)lfo_echo_time+=m;else if(l.target==MD_TO_GRAIN_POS)lfo_grain_pos+=m;else if(l.target==MD_TO_ECHO_FEEDBACK)lfo_echo_fb+=m;else if(l.target==MD_TO_GRAIN_SIZE)lfo_grain_size+=m;else lfo_mod[i][l.target]+=m;
 }
}
