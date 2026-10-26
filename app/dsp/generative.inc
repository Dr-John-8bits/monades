/* SPDX-License-Identifier: MIT OR GPL-3.0-only
 * Monades: generators (deja vu, Markov, motif), song form, gamakas. Included by dreams.cpp, inside its anonymous namespace. */
// Monades generator: a Marbles-like loop register per track. Each hit either replays its slot (chance = déjà-vu)
// or draws a new value from the source and writes it back, so the phrase mutates slowly at high déjà-vu.
// It owns its random stream: other tracks and the Mutable Dreams sequence draw exactly as before. Lock does not
// reset it (the register would never fill): on a generator track, déjà-vu is what makes the phrase repeat.
struct GenState { uint32_t rng,count,valid,gate; int32_t off[16]; int32_t prev; };
static GenState gens[6];
static void gen_reset(){for(unsigned i=0;i<6;i++){GenState &g=gens[i];memset(&g,0,sizeof g);g.rng=(patch.seed^(i*0x9e3779b9u))|1;
 const int32_t *n=patch.ext[i].notes;g.prev=0;for(int k=0;k<16;k++)if(n[k]>=0){g.prev=n[k];break;}
 stats.gen_valid[i]=stats.gen_gate[i]=0;stats.gen_pos[i]=stats.gen_fresh[i]=0;memset(stats.gen_off[i],0,sizeof stats.gen_off[i]);}}
static int gen_draw(unsigned i,GenState &g){
 const md_gen &c=patch.gen[i];unsigned r=random_word(g.rng);
 if(c.source==MD_GEN_MARKOV){
  // First-order chain learned from the phrase, read as a loop: follow one of the notes that came after the current one.
  int seq[16],n=0;for(int k=0;k<16;k++)if(patch.ext[i].notes[k]>=0)seq[n++]=patch.ext[i].notes[k];
  if(n>=2){int next[16],m=0;for(int k=0;k<n;k++)if(seq[k]==g.prev)next[m++]=seq[(k+1)%n];
   return m?next[r%m]:seq[r%n];}
 }
 if(c.source==MD_GEN_MOTIF){
  // The motif in order; with chance spread / 24 a note moves to a neighbour (up to 2 semitones, snapped later).
  int seq[16],n=0;for(int k=0;k<16;k++)if(patch.ext[i].notes[k]>=0)seq[n++]=patch.ext[i].notes[k];
  if(n){int x=seq[g.count%n];if(r%24<(unsigned)c.spread)x+=((r>>8)&1?1:-1)*(1+(int)((r>>9)&1));return x;}
 }
 int spread=c.spread;return (int)(r%(unsigned)(spread+1))-spread/2;
}
// Form: four sections, tension 0..1. Low tension thins the rhythm and lets some tracks rest, peak brings everything in.
static const float form_tension_of[MD_FORM_SECTIONS]={.25f,.6f,1.0f,.45f};
static const unsigned form_bar_len[4]={2,4,8,16};
static unsigned form_mask(unsigned cycle,unsigned section){
 if(!patch.form)return 0;unsigned live[6],n=0;for(unsigned i=0;i<6;i++)if(!patch.track[i].mute)live[n++]=i;
 unsigned drop=(unsigned)((1-form_tension_of[section&3])*patch.form_drops+.5f);if(n<2)return 0;if(drop>n-1)drop=n-1;
 unsigned r=(cycle*4+section+1)*0x9e3779b9u,mask=0; // the same every pass of this section, other tracks rest next cycle
 for(unsigned k=0;k<drop;k++){random_word(r);unsigned j=k+r%(n-k),tmp=live[k];live[k]=live[j];live[j]=tmp;mask|=1u<<live[k];}
 return mask;
}
// Gamaka shapes, shared with the screen. amount 0..1000 sets the time scale and depth.
static float gamaka_shape(int style,int amount,float from,float ms){
 float a=amount*.001f;
 switch(style){
  case MD_GAMAKA_KAN:{float hold=15+60*a,slide=8+20*a;return ms<hold?from:ms<hold+slide?from*(1-(ms-hold)/slide):0;}
  case MD_GAMAKA_MEEND:{float span=40+460*a;if(ms>=span)return 0;float x=ms/span;return from*(1-x)*(1-x)*(1+2*x);} // smooth arrival
  case MD_GAMAKA_ANDOLAN:{float rate=4.5f-2.5f*a,depth=.25f+.75f*a,onset=ms<120?ms/120:1;return from*depth*onset*(.5f-.5f*plaits::Sine(ms*.001f*rate+.25f));} // from < 0: sways down
  default:return 0;
 }
}
static float scale_neighbour(float note,int dir){ // the next scale note above (dir 1) or below (-1)
 for(int k=1;k<=4;k++){float q=quantize_any(note+dir*k,patch.root,patch.scale,dir<0);if(dir>0?q>note+.01f:q<note-.01f)return q;}
 return note+dir;
}
