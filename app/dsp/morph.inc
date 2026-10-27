// SPDX-License-Identifier: GPL-3.0-only
// Monades morph: from the patch as it was (A) towards a Monde (B). Every word of md_patch has a kind: numbers glide,
// choices switch, and the rest (mutes, lock, quality, names) stays as the player left it. A track's choices (engine,
// filter mode, LFO shapes, generator...) switch at its own point, one track after the other, and the track fades out
// before and back in after it, so the old engine is silent when the new one starts. Shared choices (scale, seed)
// switch half way. A scope keeps everything outside it on A. PLAY can carry the morph over 4 to 64 bars, one step at a time.
static md_patch morph_a,morph_b;
static int morph_pos,morph_bars,morph_scope,morph_target,morph_span;static float morph_f; // span: steps of a drift glide (0: K3 bars)
enum { MK_KEEP, MK_GLIDE, MK_SWITCH };
static uint8_t morph_kind[sizeof(md_patch)/4]; // kind | scope << 2 (MD_MORPH_SOUND, _RHYTHM, _FX)
static uint8_t morph_track[sizeof(md_patch)/4]; // the track a word belongs to, 6 = shared
enum { MORPH_FADE=60 }; // each side of a track's switch point, in thousandths of the morph
static int morph_point(int track){return track<6?170+track*132:500;} // 170, 302 ... 830; shared choices at 500
static int mk_track=6;
static void mk(size_t off,unsigned words,int kind,int scope){for(unsigned k=0;k<words;k++){morph_kind[off/4+k]=(uint8_t)(kind|scope<<2);morph_track[off/4+k]=(uint8_t)mk_track;}}
#define MK(field,words,kind,scope) mk(offsetof(md_patch,field),words,kind,scope)
static void morph_table(){
 static int built;if(built)return;built=1; // unlisted words: MK_KEEP
 const int S=MD_MORPH_SOUND,R=MD_MORPH_RHYTHM,F=MD_MORPH_FX;
 MK(seed,1,MK_SWITCH,R);MK(macro[0],2,MK_GLIDE,R);MK(macro[2],1,MK_GLIDE,S);MK(macro[3],1,MK_GLIDE,F); // density, motion / colour / space
 MK(tempo,1,MK_GLIDE,R);MK(grain_size,3,MK_GLIDE,F); // grain size, position, feedback
 for(int i=0;i<6;i++){
  mk_track=i;const size_t t=offsetof(md_patch,track)+i*sizeof(md_track);
  #define TK(field,kind,scope) mk(t+offsetof(md_track,field),1,kind,scope)
  TK(model,MK_SWITCH,S);TK(tone,MK_GLIDE,S);TK(harm,MK_GLIDE,S);TK(decay,MK_GLIDE,S);TK(level,MK_GLIDE,S);TK(send,MK_GLIDE,F);
  TK(hits,MK_GLIDE,R);TK(length,MK_GLIDE,R);TK(rotate,MK_GLIDE,R);TK(prob,MK_GLIDE,R);TK(note,MK_GLIDE,S);TK(pan,MK_GLIDE,S);
  TK(speed,MK_SWITCH,R);TK(attack,MK_GLIDE,S);TK(ratchet,MK_SWITCH,R);TK(accent,MK_GLIDE,R);TK(shape,MK_GLIDE,S);
  TK(cutoff,MK_GLIDE,S);TK(resonance,MK_GLIDE,S);TK(filter,MK_SWITCH,S);TK(breathe,MK_GLIDE,S);
  #undef TK
  const size_t e=offsetof(md_patch,ext)+i*sizeof(md_extension);
  #define EX(field,words,kind,scope) mk(e+offsetof(md_extension,field),words,kind,scope)
  EX(delay_send,2,MK_GLIDE,F);EX(bass_mode,2,MK_SWITCH,R);EX(glide,5,MK_GLIDE,S);EX(notes,16,MK_SWITCH,R); // glide, Marbles, Tides x2, Warps
  #undef EX
  for(int s=0;s<2;s++){const size_t l=offsetof(md_patch,lfo)+(i*2+s)*sizeof(md_lfo);mk(l,3,MK_SWITCH,S);mk(l+offsetof(md_lfo,depth),1,MK_GLIDE,S);}
  const size_t g=offsetof(md_patch,gen)+i*sizeof(md_gen);mk(g,1,MK_SWITCH,R);mk(g+4,3,MK_GLIDE,R); // source / deja vu, loop, spread
  mk(offsetof(md_patch,gamaka)+4*i,1,MK_SWITCH,S);mk(offsetof(md_patch,gamaka_amount)+4*i,1,MK_GLIDE,S);
 }
 mk_track=6;
 MK(root,2,MK_SWITCH,S);MK(transpose,1,MK_GLIDE,S);MK(swing,1,MK_GLIDE,R);MK(grain_pitch,1,MK_GLIDE,F);MK(reverse,2,MK_SWITCH,F);MK(duck_amount,2,MK_GLIDE,F);
 MK(delay_ms,7,MK_GLIDE,F); // echo time / feedback, room tail, return, send high-pass, echo / room tone
 MK(echo_sync,1,MK_SWITCH,F);MK(room_diffusion,3,MK_GLIDE,F);
 MK(form,1,MK_GLIDE,R);MK(form_bars,2,MK_SWITCH,R);MK(vadi,2,MK_SWITCH,S);MK(motion_every,1,MK_SWITCH,R);
}
#undef MK
static float morph_fade(int track,int pos){int d=pos-morph_point(track);d=d<0?-d:d;return d>=MORPH_FADE?1.0f:(float)d/MORPH_FADE;}
static unsigned morph_changes; // bit per track: one of its choices differs between A and B (in scope): it fades at its point
static void morph_apply(bool fade=true){
 const int32_t *a=reinterpret_cast<const int32_t*>(&morph_a),*b=reinterpret_cast<const int32_t*>(&morph_b);int32_t *p=reinterpret_cast<int32_t*>(&patch);
 const float t=morph_pos*.001f;morph_changes=0;
 for(unsigned k=0;k<sizeof(md_patch)/4;k++){const int kind=morph_kind[k]&3,scope=morph_kind[k]>>2,tk=morph_track[k];
  if(kind==MK_KEEP)continue;if(morph_scope&&scope!=morph_scope){p[k]=a[k];continue;}
  if(kind==MK_SWITCH&&tk<6&&a[k]!=b[k])morph_changes|=1u<<tk;
  p[k]=kind==MK_GLIDE?a[k]+(int32_t)floorf((float)(b[k]-a[k])*t+.5f):morph_pos>=morph_point(tk)?b[k]:a[k];}
 if(fade)for(unsigned j=0;j<6;j++)if((morph_changes>>j)&1)patch.track[j].level=(int32_t)(patch.track[j].level*morph_fade(j,morph_pos)+.5f);
 for(unsigned j=0;j<6;j++){md_track &tr=patch.track[j];tr.length=bound(tr.length,1,32);tr.hits=bound(tr.hits,0,tr.length);tr.rotate%=tr.length;
  patch.tone_preset[j]=morph_pos==0?morph_a.tone_preset[j]:morph_pos==1000&&!morph_scope?morph_b.tone_preset[j]:-1;
  if(voices[j].model!=tr.model)reset_voice(j);} // an engine switch starts a fresh voice, as ENV does
 patch.biome_id=morph_pos==0?morph_a.biome_id:morph_pos==1000&&!morph_scope?morph_target:-1;if(morph_pos)patch.edited_mask|=1u;
}
static void morph_begin(const md_patch &b,int id){morph_table();morph_a=patch;morph_b=b;morph_target=id;morph_pos=0;morph_f=0;morph_on=1;morph_apply();}
static void morph_to(int id){drift_on=0;morph_span=0;previous=patch;morph_begin(bank_patch(id),id);} // ENV goes back to A
static void morph_keep(){if(morph_on)morph_apply(false);morph_on=0;} // the patch stays as it is, at full level; previous is still A
static void morph_back(){if(morph_on){morph_pos=0;morph_f=0;}} // after ENV: the morph restarts from A
static int morph_len(int bars){static const int len[6]={0,4,8,16,32,64};return len[bound(bars,0,5)];}
static void morph_step(){
 const int span=morph_span?morph_span:morph_len(morph_bars)*16;
 if(!morph_on||!span||morph_pos>=1000)return;
 morph_f+=1000.0f/span;if(morph_f>1000)morph_f=1000;
 if((int)morph_f!=morph_pos){morph_pos=(int)morph_f;morph_apply();}
}
