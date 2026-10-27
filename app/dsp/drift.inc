// SPDX-License-Identifier: GPL-3.0-only
// Monades DERIVE: the Monde lives on its own. It glides (with the morph engine) towards a variant of itself drawn around
// its home, then towards the next one, for as long as PLAY runs. Variants stay on a leash: they are drawn around home,
// never around where the drift has got to. Now and then a track rests and comes back, new notes arrive, or (VOYAGE)
// the drift visits another Monde. Scale, root, transpose and levels never drift; the tempo stays within 10 % of home.
static md_patch drift_home;
static int drift_amount=1,drift_voyage,drift_scope,drift_count,drift_rest=-1,drift_distance;
static float drift_seconds;static unsigned drift_rng=0x6a09e667u;
static md_drift_event drift_log[3];
static int drift_len(int slow){static const int bars[7]={0,8,16,32,64,128,256};return bars[bound(slow,0,6)];}
static int drift_rand(int lo,int hi){return lo+(int)(random_word(drift_rng)%(unsigned)(hi-lo+1));}
static void drift_note(int kind,int track,int value,int up){drift_log[2]=drift_log[1];drift_log[1]=drift_log[0];drift_log[0]={kind,track,value,up};}
static bool drift_in(int scope){return !drift_scope||drift_scope==scope;}
// A number drawn around its home value, within its musical range: near, middle or far.
static void drift_set(int32_t &v,int home,int lo,int hi){static const int width[3]={100,250,450};const int w=width[bound(drift_amount,0,2)];v=bound(home+drift_rand(-w,w),lo,hi);}
// The next variant, drawn straight into b (morph_b): no patch-sized copy on the audio thread's stack. now: the patch before.
static void drift_target(md_patch &b,const md_patch &now){
 b=drift_home;const md_patch &h=drift_home;
 // VOYAGE: every fourth glide heads for another Monde (of the home's family, or any), the next one comes back.
 drift_count++;
 if(drift_voyage&&!drift_scope&&drift_count%4==2){int id=-1;
  for(int tries=0;tries<40&&id<0;tries++){int c=drift_rand(0,MD_BIOMES-1);if(c==MD_BLANK_BIOME||c==h.biome_id)continue;
   if(drift_voyage==1&&h.biome_id>=0&&biome_bank[c].family!=biome_bank[h.biome_id].family)continue;id=c;}
  if(id>=0){bank_patch(b,id);b.root=h.root;b.scale=h.scale;b.transpose=h.transpose;for(int j=0;j<6;j++)b.track[j].mute=h.track[j].mute;drift_note(5,-1,id,0);}}
 const md_patch &base=b; /* the same patch: every value is read once, before its own draw */const int S=MD_MORPH_SOUND,R=MD_MORPH_RHYTHM,F=MD_MORPH_FX;
 for(int j=0;j<6;j++){md_track &t=b.track[j];const md_track &o=base.track[j];
  if(drift_in(S)){drift_set(t.tone,o.tone,0,1000);drift_set(t.harm,o.harm,0,1000);drift_set(t.shape,o.shape,0,1000);
   drift_set(t.cutoff,o.cutoff,400,1000);drift_set(t.resonance,o.resonance,0,650);drift_set(t.decay,o.decay,150,900);
   drift_set(t.attack,o.attack,0,400);drift_set(t.pan,o.pan,150,850);
   for(int s=0;s<2;s++)if(base.lfo[j][s].depth){const int d=base.lfo[j][s].depth;drift_set(b.lfo[j][s].depth,d,d<0?-1000:0,d<0?0:1000);}}
  if(drift_in(R)){drift_set(t.prob,o.prob,550,1000);drift_set(t.accent,o.accent,0,800);
   if(o.hits)t.hits=bound(o.hits+drift_rand(-1-drift_amount,1+drift_amount),1,o.length);} // a track left silent stays silent
  if(drift_in(F)){drift_set(t.send,o.send,0,700);drift_set(b.ext[j].delay_send,base.ext[j].delay_send,0,600);drift_set(b.ext[j].reverb_send,base.ext[j].reverb_send,0,600);}
 }
 if(drift_in(S))drift_set(b.macro[2],base.macro[2],200,800);
 b.tempo=drift_in(R)?bound(h.tempo+drift_rand(-h.tempo/10,h.tempo/10),30,220):h.tempo;
 if(drift_in(R))drift_set(b.macro[1],base.macro[1],0,600);
 if(drift_in(F)){drift_set(b.grain_size,base.grain_size,100,900);drift_set(b.grain_pos,base.grain_pos,0,1000);drift_set(b.feedback,base.feedback,0,600);
  drift_set(b.delay_fb,base.delay_fb,0,600);drift_set(b.reverb_time,base.reverb_time,200,850);drift_set(b.echo_tone,base.echo_tone,200,800);
  drift_set(b.room_tone,base.room_tone,200,800);drift_set(b.room_diffusion,base.room_diffusion,0,1000);drift_set(b.chorus,base.chorus,0,600);
  b.fx_level=base.fx_level>300?base.fx_level:300;} // the effects that drift can be heard
 // Sections: a track (never the first, the anchor) rests for one glide and comes back with the next.
 if(drift_rest>=0){drift_note(3,drift_rest,0,1);drift_rest=-1;}
 else if((drift_in(R))&&drift_rand(0,3)==0){drift_rest=drift_rand(1,5);b.track[drift_rest].level=0;drift_note(2,drift_rest,0,0);}
 if(drift_in(R)&&drift_rand(0,2)==0){b.seed=random_word(drift_rng)|1;drift_note(4,-1,0,1);}
 // What moves most on a track, for the screen.
 {int best=0,bt=-1,bf=0,up=0;
  for(int j=0;j<6;j++){const int32_t from[5]={now.track[j].cutoff,now.track[j].tone,now.track[j].harm,now.track[j].shape,now.track[j].decay},to[5]={b.track[j].cutoff,b.track[j].tone,b.track[j].harm,b.track[j].shape,b.track[j].decay};
   for(int f=0;f<5;f++){int d=to[f]-from[f];if((d<0?-d:d)>best){best=d<0?-d:d;bt=j;bf=f;up=d>0;}}}
  if(bt>=0&&best>60)drift_note(1,bt,bf,up);}
}
static void drift_arm(int slow){
 if(!slow){drift_on=0;morph_on=0;morph_span=0;drift_slow=0;return;} // the patch stays where the drift left it
 if(!drift_on){drift_home=patch;previous=patch;drift_on=1;drift_seconds=0;drift_count=0;drift_rest=-1;morph_on=0;morph_scope=0;
  drift_rng=(patch.locked?patch.seed:stats.frames)^0x6a09e667u;if(!drift_rng)drift_rng=1;memset(drift_log,0,sizeof drift_log);}
 drift_slow=slow;if(morph_on)morph_span=drift_len(slow)*16;
}
// A change made while a morph runs (a knob, REC, DICE) is written into A and B, so the next step keeps it; during
// DERIVE, a knob also moves home. morph_before holds the patch from just before the command.
static md_patch morph_before;
static void morph_pin(bool to_home){
 const int32_t *was=reinterpret_cast<const int32_t*>(&morph_before),*now=reinterpret_cast<const int32_t*>(&patch);
 int32_t *a=reinterpret_cast<int32_t*>(&morph_a),*b=reinterpret_cast<int32_t*>(&morph_b),*h=reinterpret_cast<int32_t*>(&drift_home);
 for(unsigned k=0;k<sizeof(md_patch)/4;k++)if(now[k]!=was[k]&&(morph_kind[k]&3)!=MK_KEEP){a[k]=b[k]=now[k];if(to_home&&drift_on)h[k]=now[k];}
}
static void drift_new_home(){if(!drift_on)return;drift_home=patch;previous=patch;morph_on=0;drift_rest=-1;drift_note(6,-1,0,0);}
static void drift_back(){if(drift_on){morph_on=0;drift_rest=-1;drift_note(6,-1,0,0);}} // after ENV: home again, the next glide leaves from there
static void drift_step(float duration){
 if(!drift_on)return;drift_seconds+=duration/MD_RATE;
 if(!morph_on||morph_pos>=1000){drift_target(morph_b,patch);morph_span=drift_len(drift_slow)*16;morph_begin(-1);}
 // How far from home: the average gap over the drifting numbers, against the widest drift (FAR).
 long gap=0;int n=0;for(int j=0;j<6;j++){const md_track &t=patch.track[j],&o=drift_home.track[j];const int g[6]={t.tone-o.tone,t.harm-o.harm,t.cutoff-o.cutoff,t.decay-o.decay,t.shape-o.shape,t.send-o.send};for(int x:g){gap+=x<0?-x:x;n++;}}
 drift_distance=bound((int)(gap/n*1000/450),0,1000);
}
