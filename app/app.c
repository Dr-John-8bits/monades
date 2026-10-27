/* SPDX-License-Identifier: MIT OR GPL-3.0-only */
#include "dsp/dreams.h"
#include "fastmath.h"
enum { LIVE,SOUND,RHYTHM,GRAIN,MIX,SYSTEM,ENVELOPE,GROOVE,TAPE,HARMONY,FILTER,APPEARANCE,MIXER,BASSLINE,SPACE,MODULATION,PHRASE,MIDI,BANKS,ZONES,LFO_A,LFO_B,SPACE_TONE,ECHO_CHORUS,GENERATOR,FORM,RAGA,MORPH,DRIFT };
static struct { int page,track,help,dirty,octave; uint32_t btn,keys,down[NB],held,next; char msg[32]; uint32_t msg_until; } dream_ui;
static int dream_store_hold;
static volatile int dream_reloading;
static float md_audio[256],md_gain;static uint32_t midi_link;
static struct { uint32_t magic; int32_t motion,boot_sound; md_midi_routing midi; int32_t custom_scale; } dream_prefs; /* 1.1 saved only the first three words */
static int dream_midi_ok(const md_midi_routing *r){
 if(r->mode<0||r->mode>2||r->clock<0||r->clock>1)return 0;
 for(int z=0;z<8;z++)if(r->zone[z].lo<0||r->zone[z].lo>127||r->zone[z].hi<0||r->zone[z].hi>127||r->zone[z].ch<0||r->zone[z].ch>16)return 0;
 return 1;
}
static void dream_midi_defaults(md_midi_routing *r){
 // Off until chosen; three ready zones: C1-B2 channel 1, C3-B4 channel 2, C5-C7 channel 3.
 static const int32_t z[3][3]={{24,47,1},{48,71,2},{72,96,3}};memset(r,0,sizeof *r);
 for(int i=0;i<8;i++){r->zone[i].lo=i<3?z[i][0]:24;r->zone[i].hi=i<3?z[i][1]:96;r->zone[i].ch=i<3?z[i][2]:0;}
}
static uint32_t intro_until,intro_start,chime_left;static int intro_pending;static float chime_phase[3];
static void dream_intro(void){intro_start=plat_ms();intro_until=intro_start+3200;chime_left=dream_prefs.boot_sound?88200:0;for(int i=0;i<3;i++)chime_phase[i]=0;}
static uint32_t intro_left_at; // the tap that ends the intro only ends it (HOME then stays on LIVE)
static void dream_dismiss_intro(void){if(intro_until)intro_left_at=plat_ms();intro_until=chime_left=0;}
static uint32_t overloads,cpu_peak,flower_time,last_events[6],pulse_until[6],quality_notice;
// Monades: six stellar colours, one per ship of the fleet.
static const uint16_t dream_colors[6]={RGB(255,140,118),RGB(255,198,104),RGB(124,222,184),RGB(128,170,255),RGB(204,150,255),RGB(96,220,240)};
#define MD_BG RGB(6,8,20)
#define MD_CARD RGB(22,26,48)
#define MD_INK RGB(244,242,255)
#define MD_DIM RGB(172,180,208)
#define MD_EDGE RGB(60,66,100)
static const char *dream_pages[]={"LIVE MONDE","SOUND","RHYTHM","GRANULAR BUS","MIX / NOTES","QUALITY / SYSTEM","ENVELOPE / BLOOM","GROOVE / REPEATS","TAPE / MOTION","HARMONY / GLOBAL","VELVET FILTER","LOOK / STARTUP","GLOBAL MIXER","BASS / MOTIFS","SHARED SPACE","MARBLES / TIDES / WARPS","PHRASE / NOTES","MIDI / PERFORMANCE","USER BANKS","MIDI OUT ZONES","LFO A","LFO B","SPACE TONE","ECHO SYNC / CHORUS","GENERATOR / DEJA VU","FORM","RAGA / GAMAKA","MORPH","DERIVE"};
static const char *help_title[NB]={"GRANULAR BUS","MUTE A TRACK","RETURN","FREEZE","CHOOSE A VOICE","GLOBAL","LIVE MACROS","SAVE","LOCK THE SEED","GENERATE RHYTHMS","PLAY / STOP","VARY / DICE","LOWER OCTAVE","HIGHER OCTAVE"};
static const char *help_lines[NB][5]={
 {"FX: Grain > Tape > Space >","Space Tone > Echo / Chorus >","Marbles / Tides > LFO A > LFO B","Grain: K1 mix / K2 length","REC: dice this page"},
 {"SEL: mute / unmute this track","SELECT: choose one of six tracks","ALGO: change sound engine","Every engine fits every track.","EDIT + PRESET: sound library"},
 {"ENV: undo the last variation","REC: make a new variation","One previous variation is kept.","Choosing a Monde starts fresh.","SAVE: store the current patch"},
 {"LFO: freeze the granular memory","The cloud keeps playing.","Tracks remain live and playable.","LFO again: resume recording","FX: edit the cloud and tape"},
 {"EDIT: Sound/Filter/Env/Mix/Mixer","PRESET: 224 sound presets","ALGO: twelve sound engines","Rhythm and notes are preserved.","HOME + PRESET: whole Monde"},
 {"GLO: Harmony > Raga > System >","Look > MIDI > Zones > Banks","Harmony: root / scale / transpose","System: tempo / quality","SAVE stores these preferences."},
 {"K1 DENSITY: number of events","K2 MOTION: melodic movement","K3 COLOUR: brighter / darker","K4 SPACE: granular level","PRESET: 90 Mondes + Blank"},
 {"Stop PLAY; use internal clock.","SAVE stores Monde and preferences.","GLO > BANKS: 16 Mondes / 64 tones","Factory Mondes stay available.","Granular audio is not stored."},
 {"ARP: lock the random seed","The same seed repeats.","REC: create a new variation","ENV: undo the last variation","Unequal lengths keep cycling."},
 {"SEQ: Rhythm > Groove > Bass >","Phrase > Generator > Form","Rhythm: hits / length / rotation","K4: event probability","REC: dice this page"},
 {"PLAY: start / stop","Stop lets the sound decay.","SYSTEM + REC: silence all","Keyboard: play the selected track","Start restarts the sequence."},
 {"LIVE / GLO: new variation","Other pages: dice what you see","Scale, tempo, mutes stay put.","ENV: undo / SYSTEM, MIDI: PANIC","Holding opens help only."},
 {"OCT-: keyboard one octave down","Sequence notes stay unchanged.","MIX K2: the track base note","The first hardware key is F3.","HOME: return to live controls"},
 {"OCT+: keyboard one octave up","Sequence notes stay unchanged.","MIX K2: the track base note","HOME + PRESET: whole Monde","EDIT + PRESET: single sound"}
};
static int md_context_help(void){
 int p=dream_ui.page,b=dream_ui.help;
 return (p==MIXER&&b==B_EDIT)||((p==BASSLINE||p==PHRASE||p==GENERATOR||p==FORM)&&b==B_SEQ)||
        ((p==SPACE||p==MODULATION||p==LFO_A||p==LFO_B||p==SPACE_TONE||p==ECHO_CHORUS)&&b==B_FX)||
        ((p==MIDI||p==BANKS||p==RAGA||p==ZONES)&&b==B_GLO)||((p==MORPH||p==DRIFT)&&b==B_HOME)||(p==BANKS&&b==B_SAVE);
}
static int dream_cmd(int c,int t,int v){uint32_t f=plat_lock();int r=md_command(c,t,v);plat_unlock(f);return r;}
static int zone_sel;
static int motif_at=-1,motif_track;static uint32_t motif_until; /* next motif step while the engine catches up */
static void dream_apply_midi(void){uint32_t f=plat_lock();md_set_midi_routing(&dream_prefs.midi);plat_unlock(f);}
static int dream_install(const void *,unsigned);
#include "banks.c"
static void md_num(int v,char *s) {char b[12];int i=0;if(v<0){*s++='-';v=-v;}do{b[i++]='0'+v%10;v/=10;}while(v);while(i)*s++=b[--i];*s=0;}
// A Monde (Mutable Dreams called it a Biome) shows its number and family; an FDDS Monde, its number and name.
static const char *monde_label(int id) {static char s[32];char *q=s;const char *f;for(f="MONDE ";*f;)*q++=*f++;if(id<0){for(f="USER";*f;)*q++=*f++;*q=0;return s;}md_num(id+1,q);while(*q)q++;*q++=' ';f=md_biome_family(id);if(!strcmp(f,"FDDS"))f=md_biome_name(id);while(*f&&q<s+31)*q++=*f++;*q=0;return s;}
static void md_text(int x,int y,const char *s,uint16_t c) {
 // Quiet backplates keep the organic field away from small essential labels.
 if(y>=40&&y<155){uint16_t bg=dream_ui.help>=0?MD_CARD:(dream_ui.page==LIVE&&x>=94&&y==46+dream_ui.track*17)?MD_CARD:MD_BG;cv_rect(x-1,y-1,text_w(&FONT_XS,s)+2,14,bg);}
 cv_text(x,y,&FONT_XS,s,c);
}
static void md_circle(int x,int y,int r,uint16_t c,int fill) {int a,b;for(b=-r;b<=r;b++)for(a=-r;a<=r;a++){int q=a*a+b*b;if(q<=r*r&&(fill||q>=(r-1)*(r-1)))cv_pset(x+a,y+b,c);}}
// Subpixel coverage for our botanical marks. Audio never calls this renderer.
static void md_soft_point(float x,float y,uint16_t ink){
 if(x<0||y<0||x>=240||y>=240)return;
 int ix=(int)x,iy=(int)y;float fx=x-ix,fy=y-iy;
 for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){
  int px=ix+dx,py=iy+dy+cv_oy;if(px<0||px>=(int)cv_w||py<0||py>=(int)cv_h)continue;
  float a=(dx?fx:1-fx)*(dy?fy:1-fy);uint16_t old=swap16(cv_px[py*cv_w+px]);
  int red=(int)((old>>11)*(1-a)+(ink>>11)*a+.5f);
  int green=(int)(((old>>5)&63)*(1-a)+((ink>>5)&63)*a+.5f);
  int blue=(int)((old&31)*(1-a)+(ink&31)*a+.5f);
  cv_px[py*cv_w+px]=swap16((red<<11)|(green<<5)|blue);
 }
}
static uint16_t md_tint(uint16_t a,uint16_t b,unsigned mix);
// The fleet: six windowless ships. Live page: orbit radius jitters with Motion, hull size = real level (dB),
// flame = hits after Density, outline brightness = Colour, the dust ring around them = Space.
static void md_ship(float cx,float cy,float a,float len,uint16_t c,int solid,float flame,uint16_t flame_ink){
 float ux=fm_cosf(a),uy=fm_sinf(a),nx=-uy,ny=ux;
 float tx=cx+ux*len,ty=cy+uy*len,lx=cx-ux*len*.6f+nx*len*.55f,ly=cy-uy*len*.6f+ny*len*.55f,rx=cx-ux*len*.6f-nx*len*.55f,ry=cy-uy*len*.6f-ny*len*.55f;
 for(int k=0;k<=24;k++){float u=k/24.0f;md_soft_point(tx+(lx-tx)*u,ty+(ly-ty)*u,c);md_soft_point(tx+(rx-tx)*u,ty+(ry-ty)*u,c);md_soft_point(lx+(rx-lx)*u,ly+(ry-ly)*u,c);}
 if(solid)for(int k=1;k<12;k++){float u=k/12.0f;float ax=tx+(lx-tx)*u,ay=ty+(ly-ty)*u,bx=tx+(rx-tx)*u,by=ty+(ry-ty)*u;for(int m=0;m<=8;m++){float v=m/8.0f;md_soft_point(ax+(bx-ax)*v,ay+(by-ay)*v,c);}}
 float bx=cx-ux*len*.6f,by=cy-uy*len*.6f;for(int k=1;k<=(int)flame;k++)md_soft_point(bx-ux*k,by-uy*k,md_tint(MD_BG,flame_ink,(unsigned)(30-k*20/(flame+1))));
}
static int md_segments(float peak);
static unsigned fleet_mix=32; // a still fleet's brightness, 0..32 (the morph page fades A and lights B)
static void md_fleet(float cx,float cy,float radius,float spin,const md_patch *bp,int live){
 for(int j=0;j<6;j++){
  float a=j*1.04719755f-.52f+spin,r=radius+(live?bp->macro[1]*.004f*fm_sinf(spin*7+j*2.1f):0);
  float x=cx+fm_cosf(a)*r,y=cy+fm_sinf(a)*r;
  float size=live?4.5f+md_segments(md_status()->track_peak[j]*1.9f)*.25f:7;
  int len=bp->track[j].length>0?bp->track[j].length:1,hits=bp->track[j].hits+(bp->macro[0]-500)*len/1500;hits=hits<0?0:hits>len?len:hits;
  int pulse=dream_prefs.motion&&plat_ms()<pulse_until[j];
  float flame=live?2+hits*10.0f/len+(pulse?4:0):3;
  uint16_t ink=live?md_tint(MD_BG,dream_colors[j],(unsigned)(14+bp->macro[2]*18/1000)):md_tint(MD_BG,dream_colors[j],fleet_mix);
  md_ship(x,y,a+1.5707963f,size,bp->track[j].mute&&live?MD_EDGE:ink,!(bp->track[j].mute&&live),bp->track[j].mute&&live?0:flame,pulse?MD_INK:dream_colors[j]);
  if(live&&j==dream_ui.track)md_circle((int)x,(int)y,(int)size+4,dream_colors[j],0);
 }
}
// RGB565 translucency over the current 40-line tile: no second framebuffer.
static uint16_t md_tint(uint16_t a,uint16_t b,unsigned mix){
 unsigned rb=(((a&0xf81fu)*(32-mix)+(b&0xf81fu)*mix)>>5)&0xf81fu;
 unsigned g=(((a&0x07e0u)*(32-mix)+(b&0x07e0u)*mix)>>5)&0x07e0u;
 return (uint16_t)(rb|g);
}
// Hull panel: translucent plate with two cut corners, a colour stripe and a running light. Same 108 x 32 footprint.
static void md_panel(int x,int y,uint16_t c){
 for(int row=0;row<32;row++){
  int py=y+row+cv_oy;if(py<0||py>=(int)cv_h)continue;
  int left=row<5?5-row:0,right=row>26?row-26:0;
  int a=x+left,z=x+107-right;if(a<0)a=0;if(z>=(int)cv_w)z=cv_w-1;
  for(int px=a;px<=z;px++){uint16_t old=swap16(cv_px[py*cv_w+px]);cv_px[py*cv_w+px]=swap16(md_tint(old,MD_CARD,14));}
  if(a<=z){cv_pset(a,y+row,MD_EDGE);cv_pset(z,y+row,MD_EDGE);}
 }
 cv_rect(x+5,y,103,1,MD_EDGE);cv_rect(x,y+31,102,1,MD_EDGE);
 cv_rect(x+1,y+7,2,18,md_tint(MD_EDGE,c,20));cv_rect(x+99,y+3,5,1,c);
}
// Straight gauge with tenth ticks; the lit part and the marker carry the value.
static void md_gauge(int x,int y,int width,int value,int lo,int hi,uint16_t c){
 int fill=clamp(value-lo,0,hi-lo)*width/(hi>lo?hi-lo:1);
 cv_rect(x,y,width+1,1,MD_EDGE);if(fill>0)cv_rect(x,y-1,fill,2,c);
 for(int k=0;k<=10;k++){int tx=x+k*width/10;cv_rect(tx,y+2,1,k%5==0?3:1,k*width/10<=fill?c:MD_EDGE);}
 cv_rect(x+fill-1,y-3,3,6,c);cv_pset(x+fill,y-1,MD_INK);cv_pset(x+fill,y,MD_INK);
}
static void md_value_span(int x,int y,const char *label,int value,int lo,int hi,uint16_t c) {
 char s[12];md_panel(x,y,c);md_text(x+6,y+2,label,MD_DIM);md_num(value,s);
 cv_text(x+7,y+14,&FONT_B,s,MD_BG);cv_text(x+6,y+13,&FONT_B,s,MD_INK);
 int start=text_w(&FONT_B,s)+12;if(start<45)start=45;if(start>78)start=78;
 md_gauge(x+start,y+23,96-start,value,lo,hi,c);
}
static void md_value(int x,int y,const char *label,int value,uint16_t c){md_value_span(x,y,label,value,0,100,c);}
static void md_named(int x,int y,const char *label,const char *value,uint16_t c){md_panel(x,y,c);md_text(x+6,y+2,label,MD_DIM);cv_text(x+7,y+15,text_w(&FONT_B,value)>96?&FONT_XS:&FONT_B,value,MD_BG);cv_text(x+6,y+14,text_w(&FONT_B,value)>96?&FONT_XS:&FONT_B,value,c);}
// Segmented 2 dB meters, -48..0 dBFS at the output: what is heard, not the knob position.
static const float meter_db[24]={0.00398107f,0.00501187f,0.00630957f,0.00794328f,0.01f,0.0125893f,0.0158489f,0.0199526f,0.0251189f,0.0316228f,0.0398107f,0.0501187f,0.0630957f,0.0794328f,0.1f,0.125893f,0.158489f,0.199526f,0.251189f,0.316228f,0.398107f,0.501187f,0.630957f,0.794328f};
#define MD_HOT RGB(236,92,82)
static float meter_hold[11];static uint32_t meter_hold_until[11];
static int md_segments(float peak){int s=0;while(s<24&&peak>=meter_db[s])s++;return s;}
// Filter page: magnitude of the voice SVF in dB at w = f / fc, for its mode (LP 24 adds a Q 0.707 stage).
static float md_filter_db(float w,float q,int mode){
 float w2=w*w,den=(1-w2)*(1-w2)+w2/(q*q),num=1;
 if(mode==2)den*=(1-w2)*(1-w2)+w2*2.0f;
 else if(mode==4)num=w2*w2;
 else if(mode==7){num=w2*w2*w2*w2;den*=(1-w2)*(1-w2)+w2*2.0f;}
 else if(mode==5)num=w2/(q*q);
 else if(mode==6){num=(1-w2)*(1-w2);if(num<1e-7f)num=1e-7f;}
 return 3.01029996f*fm_log2f(num/den);
}
// One-pole low-pass y += a (x - y), as in the echo and room loops; legacy room input high-pass (pole 0.985).
static float md_onepole_db(float f,float a){float c=fm_cosf(f*(6.2831853f/44100)),b=1-a;return 3.01029996f*fm_log2f(a*a/(1-2*b*c+b*b));}
static float md_room_hp_db(float f){float c=fm_cosf(f*(6.2831853f/44100));return 3.01029996f*fm_log2f((2-2*c)/(1-1.97f*c+.970225f));}
static void md_hz(float hz,char *s){if(hz<1){int h=(int)(hz*100+.5f);*s++='0';*s++='.';*s++='0'+h/10;*s++='0'+h%10;*s=0;}else if(hz<10){int t=(int)(hz*10+.5f);md_num(t/10,s);while(*s)s++;*s++='.';*s++='0'+t%10;*s=0;}else md_num((int)(hz+.5f),s);while(*s)s++;*s++=' ';*s++='H';*s++='Z';*s=0;}
static const char *md_pitch_names[12]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
static int md_db_y(float db){if(db>18)db=18;if(db<-36)db=-36;return 86+(int)((18-db)*46/54);}
static void md_seconds(float sec,char *s){int tenths=(int)(sec*10+.5f);md_num(tenths/10,s);while(*s)s++;*s++='.';*s++='0'+tenths%10;*s++=' ';*s++='S';*s=0;}
static void md_meter(int id,int x,int w,float peak,uint16_t c,int muted){
 uint32_t now=plat_ms();
 if(peak>=meter_hold[id]){meter_hold[id]=peak;meter_hold_until[id]=now+1200;}else if(now>meter_hold_until[id])meter_hold[id]*=.85f;
 int lit=md_segments(peak),hold=md_segments(meter_hold[id]);
 for(int s=0;s<24;s++){
  uint16_t ink=MD_CARD;
  if(s<lit)ink=muted?MD_EDGE:s>=23?MD_HOT:s>=20?MD_INK:c;
  else if(s==hold-1&&hold>0)ink=s>=23?MD_HOT:MD_DIM;
  cv_rect(x,119-s*3,w,2,ink);
 }
}

// Starfield. The stars drift left at the tempo while the sequence plays and stand still when it stops
// (or with Motion off); a streak crosses a track's lane each time that track plays a note.
static struct {uint8_t x,y,depth;} stars[90];static int stars_ready;static float star_drift;static uint32_t star_clock;
static void md_space(void){
 if(!stars_ready){stars_ready=1;unsigned r=0x2545f491u;for(int k=0;k<90;k++){r^=r<<13;r^=r>>17;r^=r<<5;stars[k].x=(uint8_t)(r%240);stars[k].y=(uint8_t)(41+(r>>8)%180);stars[k].depth=(uint8_t)(1+(r>>16)%3);}}
 uint32_t now=plat_ms();if(md_playing()&&dream_prefs.motion)star_drift+=(now-star_clock)*md_state()->tempo*(1.0f/60000);star_clock=now; // one star-unit per beat
 for(int k=0;k<90;k++){int d=stars[k].depth,x=(int)(stars[k].x-star_drift*d*4)%240;if(x<0)x+=240;int y=stars[k].y;
  if(y+cv_oy<0||y+cv_oy>=40)continue;uint16_t c=d==3?MD_DIM:d==2?MD_EDGE:md_tint(MD_BG,MD_EDGE,18);cv_pset(x,y,c);if(d==3&&k%4==0){cv_pset(x+1,y,MD_EDGE);cv_pset(x,y+1,MD_EDGE);}}
 if(!dream_prefs.motion)return;
 for(int i=0;i<6;i++){uint32_t fired=pulse_until[i]-160;if(!pulse_until[i]||now-fired>900)continue; // pulse_until marks each new event
  float head=236-(now-fired)*.26f;int y=58+i*28;if(y+cv_oy<-1||y+cv_oy>=41)continue;
  for(int tl=0;tl<22;tl++){float x=head+tl;if(x<0||x>239)continue;md_soft_point(x,(float)y,md_tint(MD_BG,dream_colors[i],(unsigned)(28-tl)));}}
}
static void md_draw_all(void) {
 const md_patch *p=md_state();const md_track *t=&p->track[dream_ui.track];uint16_t col=dream_colors[dream_ui.track];char s[24];int i;
 if(dream_ui.help<0)md_space();
 cv_rect(0,0,240,39,MD_BG);cv_rect(10,38,220,1,MD_EDGE);cv_rect(10,38,28,1,col);
 cv_rect(0,224,240,16,MD_BG);
 cv_text(10,5,&FONT_B,"MONADES",MD_INK);
 // Transport, in the usual symbols: a play triangle while the sequence runs (its dot lights on every beat), a stop square
 // when it does not. PLAY starts and stops; there is no pause on the FM-1.
 if(md_playing()){for(int r=0;r<9;r++)cv_rect(222,6+r,1+(r<5?r:8-r),1,dream_colors[2]);if(md_status()->step%4==0)cv_rect(232,9,3,3,MD_INK);}
 else cv_rect(222,7,8,8,MD_DIM);
 // A new variation (REC, or every 32 steps when the seed is not locked): a circular arrow before the tempo for 1.2 s.
 {static uint32_t seed_seen,seed_at;uint32_t sd=p->seed,now=plat_ms();if(sd!=seed_seen){if(seed_seen)seed_at=now;seed_seen=sd;}
  if(seed_at&&now-seed_at<1200){for(int k=0;k<22;k++){float a=.6f+k*.24f;md_soft_point(172+fm_cosf(a)*4.5f,31+fm_sinf(a)*4.5f,MD_INK);}cv_rect(175,25,3,1,MD_INK);cv_rect(177,25,1,3,MD_INK);}}
 md_text(10,25,dream_pages[dream_ui.page],col);
 // Pages that set the selected track without the TRACK header: its number after the title.
 if(dream_ui.page==BASSLINE||dream_ui.page==PHRASE||dream_ui.page==GENERATOR||dream_ui.page==MODULATION){char tt[3]={'T',(char)('1'+dream_ui.track),0};md_text(18+text_w(&FONT_XS,dream_pages[dream_ui.page]),25,tt,dream_colors[dream_ui.track]);}
 md_num(p->tempo,s);md_text(186,25,s,MD_INK);md_text(211,25,"BPM",MD_DIM);
 if(intro_until&&plat_ms()<intro_until&&dream_ui.help<0){
  float progress=(plat_ms()-intro_start)/3200.0f;
  // The fleet arrives from deep space and takes its formation around the star.
  {float e=progress>1?1:progress,arrive=1-(1-e)*(1-e);md_circle(120,100,(int)(2+4*arrive),MD_INK,1);
   md_fleet(120,100,30+(1-arrive)*24,progress*1.2f,md_state(),0);}
  cv_text(120-text_w(&FONT_B,"MONADES")/2,158,&FONT_B,"MONADES",MD_INK);md_text(120-text_w(&FONT_XS,"FLASH DOG DUKE SILVER")/2,180,"FLASH DOG DUKE SILVER",col);
  md_text(120-text_w(&FONT_XS,"90 MONDES / 224 TONES")/2,200,"90 MONDES / 224 TONES",MD_DIM);
  cv_rect(32,220,(int)(176*progress),2,col);md_text(48,223,"ANY CONTROL TO BEGIN",MD_DIM);return;
 }
 if(dream_ui.help>=0) {
  cv_rect(6,43,228,178,MD_CARD);cv_text(14,51,&FONT_B,md_context_help()?dream_pages[dream_ui.page]:help_title[dream_ui.help],col);
  const char *const *lines=help_lines[dream_ui.help];
  static const char *soundhelp[]={"PRESET: sound / ALGO: engine","K1: Tone / brightness","K2: Harmonics / chord","K3: Shape; drums use Decay","K4: Level / EDIT: Filter"};
  static const char *elementhelp[]={"PRESET: sound / ALGO: engine","K1 strike colour / K2 geometry","K3 Mallet / Plectrum / Particles","ENVELOPE: resonator decay","K4 level / EDIT: filter"};
  static const char *mixhelp[]={"K1: send to granular bus","K2: track base note","K3: pan left / right","K4: clock division, 1 to 8","PRESET: sound / SEL: mute"};
  static const char *lookhelp[]={"K1: animation on / off","K2: startup sound on / off","K3: replay startup animation","K4: reset measured CPU peak","Stop + SAVE: keep preferences"};
  static const char *systemhelp[]={"K1: Tempo / K2: ECO or FULL","K3: ducking recovery","K4: reset measured CPU peak","REC: panic / clear tape","GLO: animation and startup"};
  static const char *harmonyhelp[]={"K1: root note C to B","K2: scale (91, by family)","K3: transpose +/-24 semitones","K4: tempo for all tracks","GLO: continue to Raga"};
  static const char *filterhelp[]={"K1: cutoff, 60 to 18000 Hz","K2: resonance at the cutoff","K3: 8 modes, LP / HP / BP / NOTCH","K4: slow filter movement","LPG darkens with the decay"};
  static const char *envhelp[]={"K1: attack up to 8 seconds","K2: decay up to 16 seconds","FM / VA / Harmonic / Choir","K3: level / K4: grain send","Drums and Rings: engine decay"};
  static const char *groovehelp[]={"K1: global swing (0-60)","K2: 1-4 repeats per hit","K3: accent / K4: clock division","Repeats divide one step.","SEQ: return to hits / length"};
  static const char *tapehelp[]={"K1: grain pitch +/-24","K2: forward / reverse","K3: duck source; 0 means off","K4: attenuate other tracks","The source stays unchanged."};
  if(dream_ui.help==B_EDIT&&dream_ui.page==SOUND)lines=t->model==MD_ELEMENTS?elementhelp:soundhelp;
  if(dream_ui.help==B_EDIT&&dream_ui.page==MIX)lines=mixhelp;
  if(dream_ui.help==B_GLO&&dream_ui.page==APPEARANCE)lines=lookhelp;
  if(dream_ui.help==B_GLO&&dream_ui.page==SYSTEM)lines=systemhelp;
  if(dream_ui.help==B_GLO&&dream_ui.page==HARMONY)lines=harmonyhelp;
  if(dream_ui.help==B_EDIT&&dream_ui.page==FILTER)lines=filterhelp;
  if(dream_ui.help==B_EDIT&&dream_ui.page==ENVELOPE)lines=envhelp;
  if(dream_ui.help==B_SEQ&&dream_ui.page==GROOVE)lines=groovehelp;
  if(dream_ui.help==B_FX&&dream_ui.page==TAPE)lines=tapehelp;
  static const char *newhelp[17][5]={
   {"SELECT: track / SEL: mute","K1 level / K2 grain send","K3 delay send / K4 reverb send","FX: SPACE sets effect return","Sends follow track level."},
   {"K1 OFF / EUCLID / STEP","K2 motif / K3 glide / K4 note","SEQ again: edit phrase notes","Marbles varies offbeat notes.","All tracks remain assignable."},
   {"K1 delay time: 40-350 ms","K2 echo feedback / K3 room tail","K4 shared effect return","Mixer: individual track sends","PANIC clears all effect tails."},
   {"K1 Marbles: remembered variation","K2 Tides: filter movement","K3 Tides cycle / K4 Warps blend","Marbles: EUCLID/STEP phrase mode","For Tides, enable a track filter."},
   {"K1 step 1-16 / K2 semitones","-1 is a rest; scale still applies","K3 accent / K4 probability","Bass page: enable this phrase","STEP mode ignores Euclidean gates."},
   {"CH 1-6 in: tracks / vel + bend","K1 clock in / K2 scale snap","K3 out: OFF / TRACKS / ZONES","K4 clock out / GLO: zones","CC7 level / 74 filter / 91 room"},
   {"K1: slot / K2: Monde or Tone","K3 load / K4 name cursor","SAVE: save; repeat to overwrite","Stop first; use internal clock","PRESET: edit name character"},
   {"K1 zone 1-8 / K2 lowest note","K3 highest note / K4 channel","Channel 0: the zone is off.","Overlapping zones send twice.","MIDI page: K3 OUT on ZONES."},
   {"K1 rate: synced 8 bars-1/32, free Hz","K2 shape: sine to sample & hold","K3 target: filter, level, pitch...","K4 depth; negative inverts","Synced LFOs restart with PLAY."},
   {"K1 rate: synced 8 bars-1/32, free Hz","K2 shape: sine to sample & hold","K3 target: filter, level, pitch...","K4 depth; negative inverts","Two LFOs per track: A and B."},
   {"K1 high-pass on echo + room sends","K2 echo tone / K3 room tone","K4 room diffusion: softer onset","Curves: what reaches the effects.","Strip: the room onset, smeared."},
   {"K1 echo synced to the tempo","Slow tempo: half the division.","K2 echo feedback","K3 chorus mix / K4 sweep depth","Chorus acts on the dry tracks."},
   {"K1 off / random / Markov / motif","K2 deja vu: 0 new, 100 frozen","K3 loop / K4 spread or variation","KEYS write the motif, REC clears","Markov and MOTIF read the phrase."},
   {"K1 form depth; 0 is off","K2 section length in bars","K3 tracks resting when calm","K4 Motion: new every N steps","Calm, build, peak, release."},
   {"K1 vadi / K2 samvadi: pivots","Pivots attract notes, accented.","K3 gamaka of this track","K4 gamaka amount","Kan, meend, andolan or mix."},
   {"K1 / PRESET: Monde B","K2 position A > B","K3 auto: PLAY carries it","K4 what morphs","REC keeps / ENV back to A"},
   {"K1 how far from home","K2 glide length; OFF stops","K3 voyage: stay, family, any","K4 what drifts","REC new home / ENV home"}};
  if(md_context_help())lines=newhelp[dream_ui.page-MIXER];
  for(i=0;i<5;i++)md_text(14,80+i*22,lines[i],MD_INK);
  md_text(14,197,"Release, then tap any key to close.",MD_DIM);return;
 }
 if(dream_ui.page>=MIXER){
  const md_extension *e=&p->ext[dream_ui.track];
  if(dream_ui.page==MIDI){
   // Activity: channels 1-6 light with their track on every note; each port's IN / OUT lights for 200 ms per message.
   {cv_rect(4,44,232,108,MD_BG);md_text(12,50,"CH",MD_DIM);
    for(i=0;i<6;i++){int x=40+i*30;uint16_t c=plat_ms()<pulse_until[i]+120?dream_colors[i]:MD_CARD;cv_rect(x,48,24,16,c);char n[3];md_num(i+1,n);cv_text(x+9,50,&FONT_XS,n,c==MD_CARD?MD_DIM:MD_BG);}
    static const char *ports[3]={"USB","BLE","TRS"};md_text(80,74,"IN",MD_DIM);md_text(140,74,"OUT",MD_DIM);
    for(i=0;i<3;i++){int y=92+i*16;md_text(12,y-5,ports[i],MD_DIM);
     for(int o=0;o<2;o++){uint32_t ago=plat_midi_ago(i,o);md_circle(86+o*60,y,4,ago<200?MD_INK:MD_CARD,1);char n[12];md_num((int)plat_midi_count(i,o),n);md_text(96+o*60,y-5,n,MD_EDGE);}}
    md_text(12,139,"CC7 LEVEL / 74 FILTER / 91 ROOM",MD_DIM);}
   md_named(8,158,"K1 CLOCK IN",md_clock_source()==0?"INTERNAL":md_clock_source()==1?"USB":"TRS",col);md_named(124,158,"K2 SCALE SNAP",md_midi_quantize()?"ON":"OFF",col);
   static const char *out_modes[3]={"OFF","TRACKS 1-6","ZONES"};
   md_named(8,194,"K3 NOTES OUT",out_modes[dream_prefs.midi.mode],col);md_named(124,194,"K4 CLOCK OUT",!dream_prefs.midi.clock?"OFF":md_clock_source()?"EXT: OFF":"ON",col);
  }else if(dream_ui.page==ZONES){
   // Note zones on the keyboard (C1..C7): one row per zone, selected zone bright, generated notes as dots under the keys.
   const md_midi_routing *r=&dream_prefs.midi;const int x0=12,base=120;cv_rect(4,44,232,108,MD_BG);
   for(int n=24;n<=96;n++){int x=x0+(n-24)*3,black=(0x54a>>(n%12))&1;cv_rect(x,base-12,2,black?6:12,black?MD_EDGE:MD_DIM);}
   for(int o=0;o<=6;o++){char lab[4]="C";md_num(o+1,lab+1);md_text(x0+o*36-2,base+2,lab,MD_DIM);}
   int overlap=0;
   for(int zi=0;zi<8;zi++){const int ch=r->zone[zi].ch,lo=r->zone[zi].lo,hi=r->zone[zi].hi,y=50+zi*7;
    uint16_t c=zi<6?dream_colors[zi]:zi==6?MD_INK:MD_DIM;
    if(zi==zone_sel)cv_rect(4,y-2,5,6,MD_INK);
    if(!ch){cv_rect(x0+(lo-24)*3,y+1,(hi-lo)*3+2,1,MD_CARD);continue;}
    cv_rect(x0+(lo-24)*3,y,(hi-lo)*3+2,zi==zone_sel?5:3,zi==zone_sel?c:md_tint(MD_BG,c,16));
    if(zi!=zone_sel&&r->zone[zone_sel].ch&&lo<=r->zone[zone_sel].hi&&hi>=r->zone[zone_sel].lo)overlap=1;}
   for(int tk=0;tk<6;tk++){int nt=md_status()->last_note[tk];if(nt<24||nt>96||plat_ms()>pulse_until[tk]+200||p->track[tk].mute)continue;md_circle(x0+(nt-24)*3,base-16,2,dream_colors[tk],1);}
   md_text(100,139,r->mode!=2?"SET MIDI K3 TO ZONES":overlap?"OVERLAP: SENT ON BOTH":"",r->mode!=2||overlap?MD_HOT:MD_DIM);
   {char lab[8];md_num(zone_sel+1,lab);md_value_span(8,158,"K1 ZONE",zone_sel+1,1,8,col);
    const char *nm=md_pitch_names[r->zone[zone_sel].lo%12];int k=0;while(nm[k]){lab[k]=nm[k];k++;}md_num(r->zone[zone_sel].lo/12-1,lab+k);md_named(124,158,"K2 LOW",lab,col);
    nm=md_pitch_names[r->zone[zone_sel].hi%12];k=0;while(nm[k]){lab[k]=nm[k];k++;}md_num(r->zone[zone_sel].hi/12-1,lab+k);md_named(8,194,"K3 HIGH",lab,col);
    if(r->zone[zone_sel].ch){md_num(r->zone[zone_sel].ch,lab);md_named(124,194,"K4 CHANNEL",lab,col);}else md_named(124,194,"K4 CHANNEL","OFF",col);}
  }else if(dream_ui.page==LFO_A||dream_ui.page==LFO_B){
   // Two cycles of this LFO as the engine computes them (random shapes show their real coming values):
   // bright = already played in this cycle, dot = now. Height = depth. Ticks under it: sixteenths / bars when synced.
   const md_stats *ms=md_status();int tr=dream_ui.track,sl=dream_ui.page==LFO_B;const md_lfo *l=&p->lfo[tr][sl];
   const int x0=16,w=208,cy=94;cv_rect(4,44,232,108,MD_BG);cv_rect(x0,cy,w,1,MD_CARD);
   float amp=8+(l->depth<0?-l->depth:l->depth)*.018f,dir=l->depth<0?-1.0f:1.0f; /* 8..26 px: the shape stays readable at small depths */
   uint32_t cyc=ms->lfo_cycle[tr][sl];float ph=ms->lfo_phase[tr][sl];
   int py=cy-(int)(md_lfo_wave(tr,sl,cyc,0)*amp*dir);
   for(i=0;i<w;i++){float pos=i*2.0f/(w-1);uint32_t k=(uint32_t)pos;if(k>1)k=1;
    int y=cy-(int)(md_lfo_wave(tr,sl,cyc+k,pos-k)*amp*dir),a=y<py?y:py,z=y<py?py:y;
    cv_rect(x0+i,a,1,z-a+1,!l->depth?MD_EDGE:pos<ph?col:md_tint(MD_BG,col,12));py=y;}
   cv_rect(x0+w/2,cy-30,1,61,MD_CARD);
   md_circle(x0+(int)(ph*(w-1)/2),cy-(int)(ms->lfo_value[tr][sl]*amp*dir),3,MD_INK,1);
   float len=md_lfo_sixteenths(l->rate);
   if(len>0){float step=(w/2.0f)/len;int every=step>=3?1:step*4>=3?4:16;
    for(int n=0;n<=(int)(2*len+.5f);n+=every){int x=x0+(int)(n*step+.5f);if(x>=x0+w)break;cv_rect(x,cy+30,1,n%16==0?7:n%4==0?5:3,n%16==0?MD_INK:MD_DIM);}}
   {char lab[24];md_num(tr+1,lab);char *q=lab;while(*q)q++;const char *nm=" > ";while(*nm)*q++=*nm++;nm=md_lfo_target_name(l->target);while(*nm)*q++=*nm++;*q=0;md_text(x0,48,lab,MD_DIM);
    md_hz(md_lfo_hz(l->rate,p->tempo),lab);md_text(x0+w-text_w(&FONT_XS,lab),48,lab,MD_DIM);}
   {const char *warn=0;int tg=l->target;
    if(!l->depth)warn="DEPTH 0: THIS LFO IS OFF";
    else if((tg==MD_TO_CUTOFF||tg==MD_TO_RESONANCE)&&!t->filter)warn="NEEDS A TRACK FILTER";
    else if((tg==MD_TO_ECHO_SEND||tg==MD_TO_ROOM_SEND||tg==MD_TO_ECHO_TIME||tg==MD_TO_ECHO_FEEDBACK)&&!p->fx_level)warn="RETURN 0: SEE SPACE";
    else if(tg==MD_TO_NOTES&&t->model<MD_FM)warn="DRUM ENGINE: NO NOTES";
    else if(tg==MD_TO_ATTACK&&!md_has_envelope(t->model))warn="THIS ENGINE HAS NO ATTACK";
    else if(tg==MD_TO_GRAIN_SEND||tg==MD_TO_GRAIN_POS||tg==MD_TO_GRAIN_SIZE){if(!p->macro[3])warn="GRAIN MIX 0: SEE GRANULAR";}
    if(warn)md_text(x0,139,warn,MD_HOT);
    else{char now[24]="NOW ",*q=now+4;float v=ms->lfo_value[tr][sl]*l->depth*.001f;
     if(tg==MD_TO_CUTOFF){md_num((int)ms->cutoff_now[tr],q);while(*q)q++;*q++=' ';*q++='H';*q++='Z';*q=0;}
     else{int x=(int)(v*(tg==MD_TO_PITCH?120:tg==MD_TO_NOTES?12:tg==MD_TO_LEVEL?100:50)+(v<0?-.5f:.5f)); /* tenths of a semitone, steps, or % of the knob range */
      if(tg==MD_TO_LEVEL)x=(int)(-(l->depth<0?-l->depth:l->depth)*.05f*(1-(l->depth<0?-ms->lfo_value[tr][sl]:ms->lfo_value[tr][sl]))-.5f);
      if(x<0){*q++='-';x=-x;}else *q++='+';
      if(tg==MD_TO_PITCH){md_num(x/10,q);while(*q)q++;*q++='.';*q++='0'+x%10;*q=0;}else md_num(x,q);
      while(*q)q++;const char *u=tg==MD_TO_PITCH||tg==MD_TO_NOTES?" ST":tg==MD_TO_HITS?" % HITS":" %";while(*u)*q++=*u++;*q=0;}
     md_text(x0,139,now,MD_DIM);}}
   md_named(8,158,"K1 RATE",l->rate<MD_LFO_SYNCED?md_lfo_rate_name(l->rate):"FREE",col);md_named(124,158,"K2 SHAPE",md_lfo_shape_name(l->shape),col);
   md_named(8,194,"K3 TARGET",md_lfo_target_name(l->target),col);md_value_span(124,194,"K4 DEPTH",l->depth/10,-100,100,col);
  }else if(dream_ui.page==SPACE_TONE){
   // What reaches the effects, 20 Hz..20 kHz: bright = echo repeats 1-3 (send high-pass x echo tone^k),
   // dim = room (send high-pass x its fixed input high-pass x room tone, twice round the loop).
   const md_stats *ms=md_status();cv_rect(4,44,232,108,MD_BG);
   for(i=0;i<5;i++)cv_rect(30,md_db_y(12-12*i),180,1,i==1?MD_EDGE:MD_CARD);
   float hp=(float)md_send_hp_hz(p->send_hp);
   float ea=p->echo_tone==500?.25f:p->echo_tone<500?.04f+.21f*p->echo_tone*.002f:.25f+.65f*(p->echo_tone-500)*.002f;
   float ra=p->room_tone==500?.3f:p->room_tone<500?.05f+.25f*p->room_tone*.002f:.3f+.55f*(p->room_tone-500)*.002f;
   for(i=0;i<180;i++){float f=20.0f*fm_exp2f(i*(9.96578428f/179)),h=hp>0?md_filter_db(f/hp,.707f,4):0,e1=md_onepole_db(f,ea);
    cv_pset(30+i,md_db_y(h+md_room_hp_db(f)+2*md_onepole_db(f,ra)),MD_DIM);
    for(int k=3;k>=1;k--)cv_rect(30+i,md_db_y(h+k*e1),1,k==1?2:1,k==1?col:md_tint(MD_BG,col,(unsigned)(18-k*4)));}
   if(hp>0){int fx=30+(int)(179*fm_log2f(hp/20.0f)/9.96578428f);cv_rect(fx,86,1,46,md_tint(MD_BG,col,12));}
   {int sends=0;for(i=0;i<6;i++)sends+=p->ext[i].delay_send+p->ext[i].reverb_send;
    md_text(12,48,!p->fx_level?"RETURN 0: NOTHING HEARD":!sends?"NO TRACK SENDS: SEE MIXER":"BRIGHT: ECHO REPEATS  DIM: ROOM",p->fx_level&&sends?MD_DIM:MD_HOT);}
   md_text(66,137,"100",MD_DIM);md_text(128,137,"1K",MD_DIM);md_text(186,137,"10K",MD_DIM);md_text(6,md_db_y(0)-6,"0dB",MD_DIM);
   md_meter(7,214,8,ms->fx_peak[1],MD_DIM,0);md_meter(8,226,8,ms->fx_peak[2],MD_DIM,0);
   {char hz[16];if(hp>0){md_num((int)hp,hz);char *q=hz;while(*q)q++;*q++=' ';*q++='H';*q++='Z';*q=0;}else{hz[0]='O';hz[1]='F';hz[2]='F';hz[3]=0;}md_named(8,158,"K1 SEND HIGH-PASS",hz,col);}
   // Room onset: an impulse through the four input allpasses as the engine runs them (27 ms), loudest sample per column.
   {static float line[1210];static unsigned head[4];static const unsigned len[4]={150,214,319,527};float g=p->room_diffusion*.0007f;
    memset(line,0,sizeof line);memset(head,0,sizeof head);cv_rect(30,84,180,1,MD_CARD);
    if(g<=0)cv_rect(30,68,2,16,MD_DIM);else /* off: the room hears the send directly */
    for(int col_i=0,smp=0;col_i<180;col_i++){float pk=0;for(int k=0;k<7&&smp<1260;k++,smp++){float x=smp==0?1.0f:0.0f;unsigned base=0;
      for(int j=0;j<4;j++){float d=line[base+head[j]],y=d-g*x;line[base+head[j]]=x+g*y;if(++head[j]>=len[j])head[j]=0;base+=len[j];x=y;}
      if(x<0)x=-x;if(x>pk)pk=x;}
     int h=(int)(fm_sqrtf(pk)*22);if(h>16)h=16;if(h)cv_rect(30+col_i,84-h,1,h,col);}
    md_text(6,72,"IN",MD_DIM);}
   md_value(124,158,"K2 ECHO TONE",p->echo_tone/10,col);md_value(8,194,"K3 ROOM TONE",p->room_tone/10,col);md_value(124,194,"K4 DIFFUSION",p->room_diffusion/10,col);
  }else if(dream_ui.page==ECHO_CHORUS){
   // One bar of sixteenths: echo taps at the time in use (height = feedback^k); synced, they fall on the grid.
   // Chorus: the two delay taps of each side sweep these ranges (centre 18 / 27 ms, width = depth), brightness = mix.
   cv_rect(4,44,232,108,MD_BG);const int x0=16,w=208,base=100;float bar=60000.0f/p->tempo*4;
   for(i=0;i<=16;i++){int x=x0+i*(w-1)/16;cv_rect(x,base+2,1,i%4==0?6:3,i%4==0?MD_DIM:MD_EDGE);}
   cv_rect(x0,base,w,1,MD_EDGE);
   {float ms=md_echo_ms(),amp=1,fb=p->delay_fb*.001f;uint16_t ink=p->fx_level?col:MD_EDGE;
    for(int k=1;k<64&&amp>.02f;k++,amp*=fb){float tm=k*ms;if(tm>bar)break;int x=x0+(int)(tm/bar*(w-1)),h=(int)(40*amp);cv_rect(x-1,base-h,3,h,ink);}
    char lab[24];md_num((int)(ms+.5f),lab);char *q=lab;while(*q)q++;const char *u=" MS";while(*u)*q++=*u++;*q=0;md_text(x0+w-text_w(&FONT_XS,lab),48,lab,MD_DIM);
    {int halved=p->echo_sync&&ms<md_echo_sync_len16(p->echo_sync)*15000.0f/p->tempo-1;
     md_text(x0,48,!p->fx_level?"RETURN 0: SEE SPACE":halved?"SLOW TEMPO: HALF DIVISION":p->echo_sync?"ECHOES ON THE GRID":"FREE ECHO (ms)",p->fx_level?MD_DIM:MD_HOT);}}
   {const int cy=124;float depth=p->chorus_depth*.001f*384/44.1f; /* ms */
    cv_rect(x0,cy,w,1,MD_CARD);uint16_t ink=p->chorus?md_tint(MD_BG,col,(unsigned)(8+p->chorus*24/1000)):MD_EDGE;
    for(int c=0;c<2;c++){float centre=c?27.2f:18.1f;int a=x0+(int)((centre-depth)/46.4f*(w-1)),z=x0+(int)((centre+depth)/46.4f*(w-1)),y=cy-6+c*8;cv_rect(a,y,z-a+1,4,ink);cv_rect(x0+(int)(centre/46.4f*(w-1)),y-1,1,6,MD_DIM);}
    md_text(x0,139,p->chorus?"CHORUS: DELAY SWEEP 0-46 ms":"CHORUS OFF",p->chorus?MD_DIM:MD_EDGE);}
   md_named(8,158,"K1 ECHO SYNC",md_echo_sync_name(p->echo_sync),col);md_value(124,158,"K2 FEEDBACK",p->delay_fb/10,col);
   md_value(8,194,"K3 CHORUS MIX",p->chorus/10,col);md_value(124,194,"K4 CHORUS DEPTH",p->chorus_depth/10,col);
  }else if(dream_ui.page==GENERATOR){
   // The loop register as the engine holds it: one column per slot (height = note offset, hollow = Chance said rest),
   // frame = the slot of the last hit, white = it was just drawn anew. Déjà-vu decides replay vs new.
   const md_stats *ms=md_status();int tr=dream_ui.track;const md_gen *g=&p->gen[tr];
   const int x0=16,w=208,base=112,len=g->loop>0?g->loop:1,cw=w/len;cv_rect(4,44,232,108,MD_BG);
   cv_rect(x0,base,w,1,MD_EDGE);
   for(i=0;i<len;i++){int x=x0+i*cw,valid=(ms->gen_valid[tr]>>i)&1,off=ms->gen_off[tr][i],h=6+(off+12)*44/36;if(h<2)h=2;
    if(!g->source||!valid){cv_rect(x+2,base-1,cw-4>1?cw-4:1,1,MD_CARD);continue;}
    uint16_t c=(i==(int)ms->gen_pos[tr]&&ms->gen_fresh[tr])?MD_INK:col;
    if((ms->gen_gate[tr]>>i)&1)cv_rect(x+2,base-h,cw-4>1?cw-4:1,h,c);else{cv_rect(x+2,base-h,cw-4>1?cw-4:1,1,c);cv_rect(x+2,base-h,1,h,c);cv_rect(x+cw-3,base-h,1,h,c);}
    if(i==(int)ms->gen_pos[tr])cv_rect(x+1,base+3,cw-2,2,MD_INK);}
   {char lab[28];if(!g->source)lab[0]=0;else if(g->dejavu>=1000){const char *a="FROZEN LOOP";int k=0;while((lab[k]=a[k]))k++;}
    else if(!g->dejavu){const char *a="NEW NOTE EVERY HIT";int k=0;while((lab[k]=a[k]))k++;}
    else{const char *a="1 NEW IN ";int k=0;while((lab[k]=a[k]))k++;md_num((int)(1000.0f/(1000-g->dejavu)+.5f),lab+k);while(lab[k])k++;a=" HITS";while((lab[k]=*a++))k++;}
    md_text(x0,48,lab,MD_DIM);}
   {const char *warn=0,*info=0;int nn=0,mv=0;const int32_t *ph=p->ext[tr].notes;int prev=-1,first=-1;for(int k=0;k<16;k++)if(ph[k]>=0){nn++;if(prev>=0)mv++;else first=k;prev=ph[k];}if(nn>1)mv++;(void)first;
    if(!g->source)info="OFF: MOTION MACRO PLAYS (1.1)";
    else if(t->mute)warn="TRACK MUTED: SEL TO HEAR IT";
    else if(p->ext[tr].bass_mode)warn="PHRASE MODE ON: NOTES FROM PHRASE";
    else if(t->model<MD_FM)info="DRUMS: DEJA VU ON CHANCE ONLY";
    else if(g->source==MD_GEN_MARKOV&&nn<2)warn="PHRASE HAS < 2 NOTES: RANDOM";
    if(warn)md_text(x0,139,warn,MD_HOT);else if(info)md_text(x0,139,info,MD_DIM);
    else if(g->source==MD_GEN_MARKOV){char lab[32]="LEARNS PHRASE: ";md_num(nn,lab+15);char *q=lab;while(*q)q++;const char *a=" NOTES ";while(*a)*q++=*a++;md_num(mv,q);while(*q)q++;a=" MOVES";while(*a)*q++=*a++;*q=0;md_text(x0,139,lab,MD_DIM);}
    else if(g->source==MD_GEN_MOTIF)md_text(x0,139,"KEYS: WRITE MOTIF  REC: CLEAR",MD_DIM);
    else md_text(x0,139,"HEIGHT: NOTE  HOLLOW: CHANCE REST",MD_DIM);}
   // The motif (the track's phrase) the keys write into: 16 cells, cursor under the next one.
   {const int mx=148,my=60;const md_extension *me=&p->ext[tr];
    for(int k=0;k<16;k++){int v=me->notes[k],x=mx+k*5;if(v<0)cv_rect(x,my,4,1,MD_EDGE);else cv_rect(x,my-v/2-1,4,2,col);if(k==me->edit_step)cv_rect(x,my+3,4,1,MD_INK);}}
   md_named(8,158,"K1 SOURCE",md_gen_source_name(g->source),col);md_value(124,158,"K2 DEJA VU",g->dejavu/10,col);
   md_value_span(8,194,"K3 LOOP",g->loop,1,16,col);md_value_span(124,194,"K4 SPREAD",g->spread,0,24,col);
  }else if(dream_ui.page==FORM){
   // The four sections (height = tension), the cursor where the song is, and under each one the tracks that play
   // in this cycle (dim = resting there, as the engine will decide).
   const md_stats *ms=md_status();cv_rect(4,44,232,108,MD_BG);static const char *names[4]={"CALM","BUILD","PEAK","RELEASE"};
   const int x0=12,sw=54,base=110;unsigned cyc=p->form?ms->form_cycle:0;
   for(i=0;i<4;i++){int x=x0+i*sw,h=(int)(md_form_tension(i)*40),cur=p->form&&md_playing()&&(int)ms->form_section==i;
    cv_rect(x+2,base-h,sw-6,h,!p->form?MD_CARD:cur?col:md_tint(MD_BG,col,10)); // block height = tension
    md_text(x+2,base+4,names[i],cur?MD_INK:MD_DIM);
    unsigned mask=md_form_mask(cyc,i);
    for(int tk=0;tk<6;tk++){uint16_t c=p->track[tk].mute?MD_CARD:(mask>>tk)&1?MD_EDGE:dream_colors[tk];md_circle(x+5+tk*8,base+24,3,c,1);}
    if(cur){int cx=x+2+(int)(ms->form_pos*(sw-6));cv_rect(cx,base-44,1,46,MD_INK);}} /* cursor: where the song is in this section */
   {char lab[28];md_num(md_form_bars(),lab);char *q=lab;while(*q)q++;const char *a=" BARS EACH";while(*a)*q++=*a++;*q=0;
    md_text(x0,48,!p->form?"FORM OFF: SONG FLAT":lab,!p->form?MD_HOT:MD_DIM);}
   md_value(8,158,"K1 DEPTH",p->form/10,col);{char b[12];md_num(md_form_bars(),b);char *q=b;while(*q)q++;*q++=' ';*q++='B';*q++='A';*q++='R';*q++='S';*q=0;md_named(124,158,"K2 SECTION",b,col);}
   md_value_span(8,194,"K3 RESTING",p->form_drops,0,3,col);
   // One bar of steps at top right: tall ticks = where Motion may pick a new variation.
   {int n=1<<p->motion_every;for(int k=0;k<16;k++){int x=150+k*5,on=k%n==0;cv_rect(x,on?48:52,2,on?8:4,on?(p->macro[1]?col:MD_DIM):MD_EDGE);}}
   {static const char *every[5]={"EVERY STEP","2 STEPS","4 STEPS","8 STEPS","16 STEPS"};md_named(124,194,"K4 MOTION",every[p->motion_every],col);}
  }else if(dream_ui.page==RAGA){
   // Left: the scale with the vadi (thick ring) and samvadi (thin ring). Right: this track's gamaka as the engine
   // shapes it over 600 ms (semitones up / down), and the live dot when one of its notes is sounding.
   const md_stats *ms=md_status();int tr=dream_ui.track;cv_rect(4,44,232,108,MD_BG);
   {const int cx=52,cy=98;unsigned mask=md_scale_mask(p->scale);
    for(i=0;i<12;i++){float a=i*.523598776f;int x=cx+(int)(fm_sinf(a)*30),y=cy-(int)(fm_cosf(a)*30),iv=(i-p->root+12)%12;
     md_circle(x,y,(mask>>iv)&1?3:1,(mask>>iv)&1?MD_INK:MD_EDGE,1);
     if(iv==p->vadi){md_circle(x,y,7,col,0);md_circle(x,y,6,col,0);}else if(iv==p->samvadi)md_circle(x,y,6,MD_DIM,0);
     if(iv==0)md_circle(x,y,4,col,1);}
    md_text(cx-10,cy-6,md_pitch_names[p->root],MD_DIM);}
   {const int x0=104,w=124,cy=98;int style=p->gamaka[tr]==MD_GAMAKA_MIX?ms->gamaka_style[tr]:p->gamaka[tr];
    cv_rect(x0,cy,w,1,MD_EDGE);for(int k=1;k<=6;k++)cv_rect(x0+k*w/6,cy-2,1,5,MD_CARD);
    if(style){float from=style==MD_GAMAKA_KAN?2.0f:style==MD_GAMAKA_ANDOLAN?-2.0f:-5.0f;int py=cy-(int)(from*8);
     for(i=0;i<w;i++){int y=cy-(int)(md_gamaka_shape(style,p->gamaka_amount[tr],from,i*600.0f/w)*8),a=y<py?y:py,z=y<py?py:y;cv_rect(x0+i,a,1,z-a+1,col);py=y;}
     float age=ms->gamaka_age[tr];if(ms->gamaka_style[tr]&&age<600)md_circle(x0+(int)(age*w/600),cy-(int)(ms->gamaka_now[tr]*8),3,MD_INK,1);}
    else md_text(x0+20,cy-20,"NO GAMAKA",MD_DIM);
    md_text(x0,139,"600 ms",MD_DIM);}
   {char lab[24];md_num(tr+1,lab);char *q=lab;while(*q)q++;const char *a=" > GAMAKA ";while(*a)*q++=*a++;a=md_gamaka_name(p->gamaka[tr]);while(*a)*q++=*a++;*q=0;md_text(104,48,lab,MD_DIM);}
   md_text(12,48,md_scale_name(p->scale),MD_DIM);
   md_named(8,158,"K1 VADI",p->vadi<0?"OFF":md_pitch_names[(p->root+p->vadi)%12],col);md_named(124,158,"K2 SAMVADI",p->samvadi<0?"OFF":md_pitch_names[(p->root+p->samvadi)%12],col);
   md_named(8,194,"K3 GAMAKA",md_gamaka_name(p->gamaka[tr]),col);md_value(124,194,"K4 AMOUNT",p->gamaka_amount[tr]/10,col);
  }else if(dream_ui.page==MORPH){
   // Two Mondes and the patch between them: A fades on the left, B lights up on the right, the live fleet travels
   // from one to the other. Under it, what glides (tempo) and what switches half way (scale, engines).
   md_morph_state ms;const md_patch *ma,*mb;md_morph_view(&ms,&ma,&mb);cv_rect(4,44,232,108,MD_BG);
   md_drift_state dv;md_drift_view(&dv,0);
   if(dv.on){md_fleet(120,90,20,plat_ms()*.0004f,p,1);md_text(12,48,"DERIVE DRIVES THE MORPH",col);md_text(12,128,"HOME: SEE THE DERIVE PAGE",MD_DIM);}
   else if(!ms.on){md_fleet(120,90,20,plat_ms()*.0004f,p,1);md_text(12,48,monde_label(p->biome_id),MD_INK);md_text(12,128,"K1 OR PRESET: MONDE TO MORPH TO",col);}
   else{const float tt=ms.pos*.001f;const int past=ms.pos>=500;
    md_text(10,48,monde_label(ma->biome_id)+6,past?MD_DIM:MD_INK);{const char *bn=monde_label(ms.target)+6;md_text(230-text_w(&FONT_XS,bn),48,bn,past?MD_INK:MD_DIM);}
    fleet_mix=32-(unsigned)(tt*24);md_fleet(30,86,12,0,ma,0);fleet_mix=8+(unsigned)(tt*24);md_fleet(210,86,12,0,mb,0);fleet_mix=32;
    md_fleet(30+180*tt,86,18,plat_ms()*.0004f,p,1);
    const unsigned changes=md_morph_changes();cv_rect(20,110,200,1,MD_EDGE);cv_rect(120,107,1,7,MD_DIM); // half way: scale and seed switch
    for(i=0;i<6;i++)if((changes>>i)&1)cv_rect(20+md_morph_point(i)/5,107,1,7,dream_colors[i]); // where each track switches, inside its fade
    cv_rect(20+(int)(200*tt)-1,106,3,9,col);
    char a[12],b[12],lab[40];md_num(ma->tempo,a);md_num(mb->tempo,b);md_text(12,116,a,MD_DIM);md_text(230-text_w(&FONT_XS,b),116,b,MD_DIM);
    md_num(p->tempo,lab);char *q=lab;while(*q)q++;const char *z=" BPM";while(*z)*q++=*z++;*q=0;md_text(120-text_w(&FONT_XS,lab)/2,116,lab,MD_INK);
    md_text(12,129,md_scale_name(ma->scale),past?MD_EDGE:MD_INK);{const char *bs=md_scale_name(mb->scale);md_text(230-text_w(&FONT_XS,bs),129,bs,past?MD_INK:MD_EDGE);}
    // Per track: hollow = still A, full = switched to B, dim = fading around its switch (nothing to switch: a dash).
    for(i=0;i<6;i++){int x=72+i*16,d=ms.pos-md_morph_point(i);d=d<0?-d:d;uint16_t c=md_tint(MD_BG,dream_colors[i],d>=60?32u:(unsigned)(4+d*28/60));
     if(!((changes>>i)&1))cv_rect(x,144,10,4,MD_CARD);else if(ms.pos>=md_morph_point(i))cv_rect(x,142,10,8,c);else{cv_rect(x,142,10,1,c);cv_rect(x,149,10,1,c);cv_rect(x,142,1,8,c);cv_rect(x+9,142,1,8,c);}}
    md_text(12,140,"TRACKS",MD_DIM);}
   {static const char *const scope[MD_MORPH_SCOPES]={"ALL","SOUND","RHYTHM","EFFECTS"};char bars[12];int n=md_morph_bars_len(ms.bars);if(n){md_num(n,bars);char *q=bars;while(*q)q++;const char *z=" BARS";while(*z)*q++=*z++;*q=0;}
    md_named(8,158,"K1 MONDE B",ms.on?monde_label(ms.target)+6:"-",col);md_value(124,158,"K2 POSITION",ms.on?ms.pos/10:0,col);
    md_named(8,194,"K3 AUTO",n?bars:"BY HAND",col);md_named(124,194,"K4 MORPH",scope[ms.scope],col);}
  }else if(dream_ui.page==DRIFT){
   // Home, pale, in the centre; the live fleet orbits it on its leash, as far out as the sound has drifted. Under it:
   // how far the current glide has got, when the next one starts, and the last three things that moved.
   md_drift_state ds;const md_patch *home;md_drift_view(&ds,&home);cv_rect(4,44,232,108,MD_BG);
   if(!ds.on){md_fleet(120,88,20,plat_ms()*.0004f,p,1);md_text(12,48,monde_label(p->biome_id),MD_INK);md_text(12,128,"K2: HOW SLOW, THEN PLAY",col);}
   else{char tm[24];unsigned sec=(unsigned)ds.seconds;
    if(sec<3600){md_num((int)(sec/60),tm);char *q=tm;while(*q)q++;*q++=':';*q++='0'+sec%60/10;*q++='0'+sec%10;*q=0;}
    else{md_num((int)(sec/3600),tm);char *q=tm;while(*q)q++;const char *z=" H ";while(*z)*q++=*z++;*q++='0'+sec%3600/600;*q++='0'+sec%600/60;*q=0;}
    md_text(10,48,"HOME",MD_DIM);md_text(44,48,monde_label(home->biome_id)+6,MD_INK);md_text(230-text_w(&FONT_XS,tm),48,tm,MD_DIM);
    const float a=ds.seconds*.05f,fx=120+fm_cosf(a)*(30+ds.distance*.07f),fy=84+fm_sinf(a)*(12+ds.distance*.012f); // farther out = farther from home
    for(int k=1;k<6;k++)cv_rect(120+(int)((fx-120)*k/6),84+(int)((fy-84)*k/6),1,1,MD_DIM); // the leash
    fleet_mix=9;md_fleet(120,84,11,0,home,0);fleet_mix=32;md_fleet(fx,fy,13,plat_ms()*.0004f,p,1);
    cv_rect(20,112,200,1,MD_EDGE);cv_rect(20+ds.pos/5-1,109,3,7,col);
    {unsigned nx=(unsigned)ds.next;char n[20]="NEXT ";md_num((int)(nx/60),n+5);char *q=n;while(*q)q++;*q++=':';*q++='0'+nx%60/10;*q++='0'+nx%10;*q=0;md_text(230-text_w(&FONT_XS,n),116,n,MD_DIM);}
    static const char *const what[5]={"CUTOFF","TONE","HARMONICS","SHAPE","DECAY"};
    for(int k=0;k<3;k++){const md_drift_event *e=&ds.events[k];if(!e->kind)continue;char line[40],*q=line;const char *z;
     if(e->track>=0){*q++='T';*q++='1'+e->track;*q++=' ';}
     z=e->kind==1?what[e->value%5]:e->kind==2?"RESTS":e->kind==3?"COMES BACK":e->kind==4?"NEW NOTES":e->kind==5?"TO ":"HOME AGAIN";while(*z)*q++=*z++;
     if(e->kind==1){z=e->up?" UP":" DOWN";while(*z)*q++=*z++;}if(e->kind==5){z=monde_label(e->value)+6;while(*z&&q<line+38)*q++=*z++;}*q=0;
     md_text(12,116+k*12,line,k?MD_DIM:MD_INK);}}
   {static const char *const far[3]={"NEAR","MIDDLE","FAR"},*const voyage[3]={"STAY","FAMILY","ANY MONDE"},*const scope[MD_MORPH_SCOPES]={"ALL","SOUND","RHYTHM","EFFECTS"};
    char g[16];int n=md_drift_bars(ds.slow);if(n){md_num(n,g);char *q=g;while(*q)q++;const char *z=" BARS";while(*z)*q++=*z++;*q=0;}
    md_named(8,158,"K1 DISTANCE",far[ds.amount],col);md_named(124,158,"K2 GLIDE",n?g:"OFF",col);
    md_named(8,194,"K3 VOYAGE",voyage[ds.voyage],col);md_named(124,194,"K4 DRIFT",scope[ds.scope],col);}
  }else if(dream_ui.page==BANKS){
   md_text(12,66,"KEEP YOUR OWN SMALL WORLDS",MD_INK);md_text(12,96,bank_name,col);char prefix[24];memcpy(prefix,bank_name,bank_char);prefix[bank_char]=0;cv_rect(12+text_w(&FONT_XS,prefix),113,5,1,col);md_text(12,121,"STOP FIRST / INTERNAL CLOCK",MD_DIM);
   md_value_span(8,158,"K1 USER SLOT",bank_slot+1,1,bank_kind?64:16,col);md_named(124,158,"K2 BANK",bank_kind?"SOUNDS":"MONDES",col);md_named(8,194,"K3 TURN RIGHT","LOAD",col);md_value_span(124,194,"K4 NAME CURSOR",bank_char+1,1,23,col);
  }else if(dream_ui.page==MIXER){
   const md_stats *ms=md_status();cv_rect(4,46,232,108,MD_BG); // a quiet plate: meters must read cleanly
   for(i=0;i<6;i++){
    // Per track: output meter, fader position on its rail, then grain / delay / reverb sends.
    int x=8+i*28,muted=p->track[i].mute;uint16_t c=muted?MD_EDGE:dream_colors[i];const md_extension *te=&p->ext[i];
    md_meter(i,x,10,ms->track_peak[i]*1.9f,dream_colors[i],muted);
    cv_rect(x+14,50,1,72,MD_EDGE);cv_rect(x+12,119-p->track[i].level*69/1000,5,3,c);
    const int sends[3]={p->track[i].send,te->delay_send,te->reverb_send};
    for(int j=0;j<3;j++){cv_rect(x,126+j*4,18,1,MD_EDGE);cv_rect(x,126+j*4,sends[j]*18/1000,2,c);}
    md_num(i+1,s);md_text(x+5,139,s,i==dream_ui.track||plat_ms()<pulse_until[i]?MD_INK:muted?MD_EDGE:MD_DIM);
    if(i==dream_ui.track)cv_rect(x,153,18,2,col);
   }
   // Returns and master, in the same G / D / R order as the send bars.
   static const char *fx_names[3]={"G","D","R"};
   for(i=0;i<3;i++){md_meter(6+i,178+i*12,8,ms->fx_peak[i],MD_DIM,0);md_text(179+i*12,139,fx_names[i],MD_DIM);}
   md_meter(9,216,8,ms->out_peak[0]*md_gain,MD_INK,0);md_meter(10,226,8,ms->out_peak[1]*md_gain,MD_INK,0);md_text(222,139,"M",MD_INK); // M: what you hear, after MASTER (LIM watches before it)
   md_text(203,124,"LIM",ms->limit_peak>.8f?MD_HOT:MD_EDGE);
   // The four frames set the track chosen with SELECT: its number in each title.
   {static char lb[4][16];static const char *const nm[4]={"K1 LEVEL T","K2 GRAIN T","K3 DELAY T","K4 REVERB T"};for(int j=0;j<4;j++){int k=0;while(nm[j][k]){lb[j][k]=nm[j][k];k++;}lb[j][k++]='1'+dream_ui.track;lb[j][k]=0;}
   md_value(8,158,lb[0],t->level/10,col);md_value(124,158,lb[1],t->send/10,col);md_value(8,194,lb[2],e->delay_send/10,col);md_value(124,194,lb[3],e->reverb_send/10,col);}
  }else if(dream_ui.page==BASSLINE){
   md_text(12,62,"ROOTED PHRASES / ROUND LOW END",MD_INK);
   for(i=0;i<16;i++)md_circle(15+i*14,107-e->notes[i],e->notes[i]<0?1:3,col,e->notes[i]>=0);
   md_named(8,158,"K1 PHRASE",e->bass_mode==2?"STEP":e->bass_mode?"EUCLID":"OFF",col);md_value_span(124,158,"K2 MOTIF",e->pattern+1,1,4,col);md_value(8,194,"K3 GLIDE",e->glide/10,col);md_value_span(124,194,"K4 BASE NOTE",t->note,24,96,col);
  }else if(dream_ui.page==SPACE){
   // One timeline, 0..1.4 s: bars = echoes (spacing = Echo ms, height = Feedback^k), curve = room tail
   // (its four ~25 ms lines lose 1 - Room Tail factor per pass). Dim when Return is 0. Live D / R returns at right.
   const md_stats *ms=md_status();const int x0=12,w=192,base=128,hmax=72;const float span=1.4f;
   const uint16_t echo_ink=p->fx_level?col:MD_EDGE,room_ink=p->fx_level?MD_DIM:MD_EDGE;
   cv_rect(4,46,232,106,MD_BG);cv_rect(x0,base,w,1,MD_EDGE);
   for(i=1;i<4;i++)cv_rect(x0+w*i/4,base+1,1,3,MD_EDGE);
   float fb=p->delay_fb*.001f,decay=.45f+.5f*p->reverb_time*.001f,lnd=fm_logf(decay);
   {int ph=hmax;for(int px=0;px<w;px++){float tm=px*span/(w-1);int h=(int)(hmax/fm_expf(-lnd*tm/.025f)); /* decay^(t/25 ms); fm_expf only with x >= 0 */
    if(h>0||ph>0)cv_rect(x0+px,base-ph,1,ph-h+1,room_ink);ph=h;}}
   float amp=1;for(int k=1;k<64&&amp>.01f;k++,amp*=fb){float tm=k*md_echo_ms()*.001f;if(tm>span)break;int x=x0+(int)(tm/span*(w-1)),h=(int)(hmax*amp);cv_rect(x-1,base-h,3,h,echo_ink);}
   md_meter(7,214,8,ms->fx_peak[1],MD_DIM,0);md_meter(8,226,8,ms->fx_peak[2],MD_DIM,0);
   md_text(215,139,"D",MD_DIM);md_text(227,139,"R",MD_DIM);
   md_text(x0,139,"0",MD_DIM);md_text(x0+w/2-12,139,"0.7 S",MD_DIM);md_text(x0+w-30,139,"1.4 S",MD_DIM);
   int sends=0;for(i=0;i<6;i++)sends+=p->ext[i].delay_send+p->ext[i].reverb_send;
   md_text(x0,48,!p->fx_level?"RETURN 0: NOTHING HEARD":!sends?"NO TRACK SENDS: SEE MIXER":"BARS: ECHOES  CURVE: ROOM",p->fx_level&&sends?MD_DIM:MD_HOT);
   if(p->echo_sync){char nm[16];const char *a=md_echo_sync_name(p->echo_sync);int k=0;while(a[k]){nm[k]=a[k];k++;}nm[k]=0;md_named(8,158,"K1 ECHO (SYNCED)",nm,col);}else md_value_span(8,158,"K1 ECHO ms",p->delay_ms,40,350,col);md_value(124,158,"K2 FEEDBACK",p->delay_fb/10,col);md_value(8,194,"K3 ROOM TAIL",p->reverb_time/10,col);md_value(124,194,"K4 RETURN",p->fx_level/10,col);
  }else if(dream_ui.page==MODULATION){
   // Tides: one cycle of its triangle (height = amount) and the dot where this track is now.
   // Marbles: which phrase steps vary for the current seed (same rule as the sequencer). Warps: ring blend depth.
   {const md_stats *ms=md_status();int tr=dream_ui.track;cv_rect(4,44,232,108,MD_BG);
    float amt=e->tides*.001f;int x0=16,w=140,cy=74;
    cv_rect(x0,cy,w,1,MD_EDGE);
    for(i=0;i<w;i++){float ph=i/(float)(w-1),tri=1-4*(ph>.5f?ph-.5f:.5f-ph);cv_pset(x0+i,cy-(int)(tri*20*amt),t->filter?col:MD_EDGE);}
    {float ph=ms->tide_phase[tr],tri=1-4*(ph>.5f?ph-.5f:.5f-ph);md_circle(x0+(int)(ph*(w-1)),cy-(int)(tri*20*amt),3,MD_INK,1);}
    char cyc[16];md_seconds(.5f+e->tides_rate*.001f*19.5f,cyc);md_text(x0,50,"TIDES",MD_DIM);md_text(x0+w-text_w(&FONT_XS,cyc),50,cyc,MD_DIM);
    if(!t->filter)md_text(x0,82,"NEEDS A TRACK FILTER",MD_HOT);
    // Warps: the ring blend gain swings between 1 - w and 1.
    float wa=e->warps*.001f;int wx=172,ww=56;md_text(wx,50,"WARPS",MD_DIM);
    for(i=0;i<ww;i++){float g=1-wa*.5f+wa*.5f*fm_sinf(i*.3f);int h=(int)(16*g);cv_rect(wx+i,cy-h,1,2*h+1,md_tint(MD_BG,col,(unsigned)(6+wa*20)));}
    // Marbles over the 16 phrase steps.
    const md_patch *mp=md_state();int bx=16;md_text(bx,98,"MARBLES",MD_DIM);
    for(i=0;i<16;i++){int x=bx+6+i*13,y=124;
     if(i%4==0){cv_rect(x-3,y-3,7,7,MD_DIM);continue;}
     unsigned mr=mp->seed^((unsigned)tr*0x9e3779b9u)^((unsigned)(i+1)*0x85ebca6bu);mr^=mr<<13;mr^=mr>>17;mr^=mr<<5;
     int varies=mr%1000<(unsigned)e->marbles;md_circle(x,y,varies?4:2,varies?col:MD_EDGE,1);}
    md_text(bx,139,!e->bass_mode?"MARBLES ACTS ON PHRASES: BASS PAGE":"SQUARES FIXED / DOTS VARY",!e->bass_mode?MD_HOT:MD_DIM);}
   md_value(8,158,"K1 MARBLES",e->marbles/10,col);md_value(124,158,"K2 TIDES",e->tides/10,col);md_value(8,194,"K3 CYCLE",e->tides_rate/10,col);md_value(124,194,"K4 WARPS",e->warps/10,col);
  }else{
   // Mini piano-roll: 16 steps by 0..+24 semitones above the base note; rests as dashes on the floor.
   // Brightness follows Chance, step 1 grows with Accent, the frame marks the step K1 edits, the bar the step playing.
   const int x0=36,cw=12,base=128;cv_rect(4,46,232,106,MD_BG);
   for(i=0;i<=24;i+=12){cv_rect(x0,base-i*3,cw*16,1,i?MD_CARD:MD_EDGE);char lab[6]="+";md_num(i,lab+(i?1:0));md_text(8,base-i*3-6,lab,MD_DIM);}
   int active=e->bass_mode!=0,len=e->bass_mode==2?(t->length>16?16:t->length):16,playing=-1;
   if(active&&md_playing())playing=(int)((md_status()->step/(unsigned)(t->speed>0?t->speed:1))%(unsigned)len);
   unsigned mix=(unsigned)(10+t->prob*22/1000);
   for(i=0;i<16;i++){int x=x0+i*cw,v=e->notes[i];
    if(i==playing)cv_rect(x,50,cw-1,base-49,md_tint(MD_BG,col,6));
    if(i>=len){cv_rect(x+3,base+3,cw-7,1,MD_CARD);continue;}
    if(v<0)cv_rect(x+3,base-1,cw-7,1,MD_DIM);
    else{int h=3+(i==0?t->accent*4/1000:0);cv_rect(x+1,base-v*3-h+2,cw-3,h,active?md_tint(MD_BG,col,mix):MD_EDGE);}
    if(i==e->edit_step){cv_rect(x,base+5,cw-1,2,MD_INK);}
   }
   md_text(x0,139,!active?"PHRASE OFF: ENABLE ON BASS PAGE":e->bass_mode==2?"STEP MODE":"EUCLID MODE: RHYTHM GATES NOTES",active?MD_DIM:MD_HOT);
   md_value_span(8,158,"K1 STEP",e->edit_step+1,1,16,col);md_value_span(124,158,"K2 NOTE / REST",e->notes[e->edit_step],-1,24,col);md_value(8,194,"K3 ACCENT",t->accent/10,col);md_value(124,194,"K4 CHANCE",t->prob/10,col);
  }
 }else if(dream_ui.page==LIVE) {
  // The fleet around its star. Space = the dust ring, Colour = how bright the hulls are (see md_fleet).
  {float hr=42+p->macro[3]*.012f;for(int k=0;k<96;k++){float b=k*.065449847f;if(k%2)md_soft_point(51+fm_cosf(b)*hr,95+fm_sinf(b)*hr,md_tint(MD_BG,MD_DIM,(unsigned)(4+p->macro[3]*22/1000)));}}
  md_circle(51,95,4,MD_INK,1);md_circle(51,95,6,md_tint(MD_BG,MD_INK,12),0);
  md_fleet(51,95,26,flower_time*.00012f,p,1);
  for(i=0;i<6;i++){
   if(i==dream_ui.track)cv_rect(94,45+i*17,137,16,MD_CARD);
   md_num(i+1,s);md_text(98,46+i*17,s,dream_colors[i]);md_text(112,46+i*17,md_model_name(p->track[i].model),p->track[i].mute?MD_DIM:MD_INK);
   {int lit=md_segments(md_status()->track_peak[i]*1.9f)/2;for(int sg=0;sg<12;sg++)cv_rect(206+sg*2,56+i*17,1,3,sg<lit?(sg>=11?MD_HOT:dream_colors[i]):MD_CARD);}
   if(i==dream_ui.track)cv_rect(94,48+i*17,2,9,col);
  }
  md_text(12,141,md_tone_name(p->tone_preset[dream_ui.track]),col);md_text(140,141,md_biome_family(p->biome_id),MD_DIM);
  const char *labels[]={"K1 DENSITY","K2 MOTION","K3 COLOUR","K4 SPACE"};for(i=0;i<4;i++)md_value(8+(i%2)*116,158+(i/2)*36,labels[i],p->macro[i]/10,dream_colors[i]);
 } else if(dream_ui.page==SYSTEM) {
  md_text(10,48,"MONADES 0.1",MD_INK);md_text(238-text_w(&FONT_XS,"FLASH DOG DUKE SILVER"),48,"FLASH DOG DUKE SILVER",col);md_text(10,69,p->quality?"FULL  24 MODES / 4 GRAINS":"ECO / ADAPTIVE MODES / 2 GRAINS",col);
  md_text(10,93,"LOAD / MAX %",MD_DIM);
#ifdef PLAT_HOST
  md_text(139,93,"HOST ONLY",MD_INK);
#else
  md_num(plat_cpu_pct(),s);md_text(144,93,s,MD_INK);md_text(172,93,"/",MD_DIM);md_num(cpu_peak,s);md_text(186,93,s,MD_INK);
#endif
  md_text(10,116,"XRUNS",MD_DIM);md_num(plat_xruns(),s);md_text(160,116,s,MD_INK);
  md_text(10,139,"DSP MEMORY / BYTES",MD_DIM);md_num(md_memory_bytes(),s);md_text(160,139,s,MD_INK);
  md_text(10,166,"K1 TEMPO   K2 ECO / FULL",MD_INK);md_text(10,188,"REC: PANIC / CLEAR TAPE",col);
  md_text(10,211,"K3: DUCK RELEASE / K4: RESET",MD_DIM);
 } else if(dream_ui.page==APPEARANCE) {
  md_circle(120,98,3,MD_INK,1);md_fleet(120,98,28,flower_time*.0003f,p,0); /* Motion LIVING: the fleet turns; STILL: it holds */
  md_text(24,140,"SAVE KEEPS THESE SETTINGS",MD_DIM);
  md_named(8,158,"K1 MOTION",dream_prefs.motion?"LIVING":"STILL",col);md_named(124,158,"K2 START SOUND",dream_prefs.boot_sound?"ON":"OFF",col);
  md_named(8,194,"K3 INTRO","REPLAY",col);md_named(124,194,"K4 CPU MAX","RESET",col);
 } else if(dream_ui.page==HARMONY) {
  static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  // Circle of the 12 pitch classes, C at the top: lit = in the scale, coloured rings = notes the tracks play now.
  cv_rect(4,44,232,108,MD_BG);
  cv_text(16,50,&FONT_M,notes[p->root],col);md_text(16,80,md_scale_family(p->scale),MD_DIM);cv_text(16,95,text_w(&FONT_B,md_scale_name(p->scale))>118?&FONT_XS:&FONT_B,md_scale_name(p->scale),MD_INK);
  {char tr[20]="TRANSPOSE +";md_num(p->transpose,tr+(p->transpose>0?11:10));md_text(16,112,tr,MD_DIM);}
  int custom=p->scale==md_scale_custom();unsigned down=md_scale_mask_down(p->scale);
  md_text(16,139,custom?"KEYS: ADD / REMOVE NOTES":down!=md_scale_mask(p->scale)?"HOLLOW: DESCENT ONLY":"RINGS: NOTES PLAYING",custom?col:MD_DIM);
  const md_stats *ms=md_status();unsigned mask=md_scale_mask(p->scale);const int cx=176,cy=100;
  // Every scale is drawn from its degrees in cents: twelve-tone ones land on the 12 marks, microtonal ones between them.
  static float deg[32];float period=1200;int nd=md_scale_cents(p->scale,deg,32,&period);
  int micro=period!=1200;for(int k=0;k<nd&&!micro;k++)if((int)deg[k]%100)micro=1;
  for(i=0;i<12;i++){
   float a=i*.523598776f;int x=cx+(int)(fm_sinf(a)*28),y=cy-(int)(fm_cosf(a)*28);
   int iv=(i-p->root+12)%12,up=(mask>>iv)&1,dn=(down>>iv)&1,root=i==p->root;
   int lx=cx+(int)(fm_sinf(a)*42),ly=cy-(int)(fm_cosf(a)*42);
   if(period==1200)md_text(lx-text_w(&FONT_XS,notes[i])/2,ly-6,notes[i],!micro&&(up||dn)?MD_DIM:MD_EDGE);
   if(micro){md_circle(x,y,1,MD_EDGE,1);continue;}
   // Filled = in both directions, small filled = ascent only, hollow = descent only.
   if(root){md_circle(x,y,6,col,1);md_circle(x,y,7,MD_INK,0);}else if(up&&dn)md_circle(x,y,5,MD_INK,1);else if(up)md_circle(x,y,3,MD_INK,1);else if(dn)md_circle(x,y,5,MD_INK,0);else md_circle(x,y,2,MD_EDGE,1);
  }
  if(micro){
   float base=period==1200?p->root*100.0f:0;
   for(int k=0;k<nd;k++){float a=(base+deg[k])/period*6.2831853f;int x=cx+(int)(fm_sinf(a)*28),y=cy-(int)(fm_cosf(a)*28);
    if(k==0){md_circle(x,y,5,col,1);md_circle(x,y,6,MD_INK,0);}else md_circle(x,y,nd>16?2:4,MD_INK,1);}
   char info[24];md_num(nd,info);char *q=info;while(*q)q++;const char *t2=" DEGREES / ";while(*t2)*q++=*t2++;md_num((int)(period+.5f),q);while(*q)q++;*q++=' ';*q++='C';*q=0;
   md_text(16,125,info,MD_DIM);
  }
  for(int tk=0;tk<6;tk++){
   if(p->track[tk].model<MD_FM||p->track[tk].mute||plat_ms()>pulse_until[tk]+300||ms->last_note[tk]<=0)continue;
   float pc=micro?(ms->last_pitch[tk]*100.0f-(period==1200?0:p->root*100.0f)):(float)(ms->last_note[tk]%12)*100.0f;
   float a=pc/period*6.2831853f;md_circle(cx+(int)(fm_sinf(a)*28),cy-(int)(fm_cosf(a)*28),9+tk%2,dream_colors[tk],0);
  }
  md_named(8,158,"K1 ROOT",notes[p->root],col);md_named(124,158,"K2 SCALE",md_scale_name(p->scale),col);
  md_value_span(8,194,"K3 TRANSPOSE",p->transpose,-24,24,col);md_value_span(124,194,"K4 TEMPO",p->tempo,30,220,col);
 } else {
  md_num(dream_ui.track+1,s);md_text(10,47,"TRACK",MD_DIM);cv_text(48,42,&FONT_M,s,col);
  cv_rect(72,46,160,18,MD_BG);cv_text(74,48,&FONT_B,md_model_name(t->model),MD_INK);
  md_text(14,69,md_tone_name(p->tone_preset[dream_ui.track]),col);if(p->edited_mask&(2u<<dream_ui.track))md_text(210,69,"*",col);
  if(dream_ui.page==SOUND) {
   // Oscilloscope of this voice after its filter: Tone, Harmonics and Shape change the drawn wave.
   static float sc[128];md_scope(dream_ui.track,sc);
   cv_rect(4,84,232,66,MD_BG);cv_rect(20,110,200,1,MD_EDGE);
   int start=0;for(int k=1;k<64;k++)if(sc[k-1]<0&&sc[k]>=0){start=k;break;}
   float peak=0;for(int k=start;k<start+64;k++){float a=sc[k]<0?-sc[k]:sc[k];if(a>peak)peak=a;}
   if(md_status()->track_peak[dream_ui.track]<.0002f||peak<1e-5f)md_text(102,116,"SILENT",MD_DIM);
   else{float g=22.0f/peak;int py=110-(int)(sc[start]*g);
    for(int px=1;px<200;px++){float pos=start+px*63.0f/199;int k=(int)pos;float fr=pos-k,v=sc[k]+(sc[k+1]-sc[k])*fr;int y=110-(int)(v*g);
     int a=y<py?y:py,z=y<py?py:y;cv_rect(20+px,a,1,z-a+1,col);py=y;}}
   md_text(8,139,"ZOOM FOLLOWS PITCH / AUTO GAIN",MD_DIM);
   md_value(8,158,"K1 TONE",t->tone/10,col);md_value(124,158,"K2 HARMONICS",t->harm/10,col);if(t->model==MD_ELEMENTS)md_named(8,194,"K3 EXCITER",t->shape<334?"MALLET":t->shape<667?"PLECTRUM":"PARTICLES",col);else md_value(8,194,md_has_envelope(t->model)?"K3 SHAPE":"K3 DECAY",(md_has_envelope(t->model)?t->shape:t->decay)/10,col);md_value(124,194,"K4 LEVEL",t->level/10,col);
  } else if(dream_ui.page==FILTER) {
   static const char *modes[]={"OFF","LP 12 dB","LP 24 dB","LOW PASS GATE","HIGH PASS","BAND PASS","NOTCH","HP 24 dB"};
   // Real response, 20 Hz..20 kHz (log) by +18..-36 dB: dim = the knobs, colour = what the voice hears now (LFOs, LPG).
   const md_stats *ms=md_status();int tr=dream_ui.track,mode=t->filter;
   float set_fc=(float)md_cutoff_hz(t->cutoff),set_q=.707f+2.0f*(t->resonance*.001f)*(t->resonance*.001f),live_fc=ms->cutoff_now[tr],live_q=ms->q_now[tr];
   if(live_fc<20)live_fc=set_fc;if(live_q<.5f)live_q=set_q;
   cv_rect(4,84,232,66,MD_BG);
   for(i=0;i<5;i++)cv_rect(30,md_db_y(12-12*i),180,1,i==1?MD_EDGE:MD_CARD);
   for(i=0;i<180;i++){
    float f=20.0f*fm_exp2f(i*(9.96578428f/179));
    if(!t->filter){cv_rect(30+i,md_db_y(0),1,2,MD_EDGE);continue;}
    cv_pset(30+i,md_db_y(md_filter_db(f/set_fc,set_q,mode)),MD_EDGE);
    int y=md_db_y(md_filter_db(f/live_fc,live_q,mode));cv_rect(30+i,y,1,2,col);
   }
   if(t->filter){int fx=30+(int)(179*fm_log2f(live_fc/20.0f)/9.96578428f);if(fx>=30&&fx<210){cv_rect(fx,86,1,46,md_tint(MD_BG,col,14));md_circle(fx,md_db_y(md_filter_db(1,live_q,mode)),3,MD_INK,1);}}
   else md_text(96,96,"FILTER OFF",MD_DIM);
   md_text(66,137,"100",MD_DIM);md_text(128,137,"1K",MD_DIM);md_text(186,137,"10K",MD_DIM);md_text(6,md_db_y(0)-6,"0dB",MD_DIM);
   md_value_span(8,158,"K1 CUTOFF Hz",md_cutoff_hz(t->cutoff),60,18000,col);md_value(124,158,"K2 RESONANCE",t->resonance/10,col);
   md_named(8,194,"K3 CHARACTER",modes[t->filter],col);md_value(124,194,"K4 BREATHE",t->breathe/10,col);
  } else if(dream_ui.page==RHYTHM) {
   // The ring the sequencer really uses: hits include the Density macro, the playhead follows the clock division.
   // Solid = played last pass, red ring = a hit Chance refused, pale = not reached yet (brightness = Chance), outer dots = Repeats.
   {const md_stats *ms=md_status();int tr=dream_ui.track,len=t->length>0?t->length:1;
    int hits=t->hits+(p->macro[0]-500)*len/1500;hits=hits<0?0:hits>len?len:hits;
    const int cx=120,cy=112,rr=30;
    unsigned mix=(unsigned)(8+t->prob*20/1000);
    for(i=0;i<len;i++){float a=i*6.2831853f/len-1.5707963f;int x=cx+(int)(fm_cosf(a)*rr),y=cy+(int)(fm_sinf(a)*rr);
     int hit=p->ext[tr].bass_mode==2||md_hit(i,hits,len,t->rotate);uint32_t bit=1u<<i;
     if(!hit){md_circle(x,y,1,MD_EDGE,1);continue;}
     if(ms->step_played[tr]&bit)md_circle(x,y,4,col,1);
     else if(ms->step_skipped[tr]&bit)md_circle(x,y,4,MD_HOT,0);
     else md_circle(x,y,4,md_tint(MD_BG,col,mix),1);
     for(int k=1;k<t->ratchet;k++)md_circle(cx+(int)(fm_cosf(a)*(rr+5+k*4)),cy+(int)(fm_sinf(a)*(rr+5+k*4)),1,col,1);}
    if(md_playing()){int pos=(int)((ms->step/(unsigned)(t->speed>0?t->speed:1))%(unsigned)len);float a=pos*6.2831853f/len-1.5707963f;md_circle(cx+(int)(fm_cosf(a)*(rr-9)),cy+(int)(fm_sinf(a)*(rr-9)),2,MD_INK,1);}
    char hl[12];md_num(hits,hl);char *q=hl;while(*q)q++;*q++='/';md_num(len,q);md_text(cx-text_w(&FONT_XS,hl)/2,cy-6,hl,MD_INK);
    md_text(8,139,"SOLID PLAYED  RED: CHANCE SAID NO",MD_DIM);}
   md_value_span(8,158,"K1 HITS",t->hits,0,t->length,col);md_value_span(124,158,"K2 LENGTH",t->length,1,32,col);md_value_span(8,194,"K3 ROTATION",t->rotate,0,t->length-1,col);md_value(124,194,"K4 CHANCE",t->prob/10,col);
  } else if(dream_ui.page==GRAIN) {
   // The tape itself: now at the right edge, about 3 s of past to the left.
   // Bands = grains (width = Grain Size, brightness = envelope x Space), bracket = Position, line below = Feedback.
   static md_tape tv;md_tape_view(&tv);
   const int x0=8,w=224,top=86,mid=107,half=19;const float scale=(float)w/tv.length,space=p->macro[3]*.001f;
   const uint16_t ice=RGB(151,200,230),head_ink=tv.freeze?ice:col;
   cv_rect(4,top-2,232,52,MD_BG);
   for(i=0;i<4;i++){if(!tv.grain[i].active)continue;
    int gx=x0+w-1-(int)(tv.grain[i].behind*scale),gw=(int)(tv.grain[i].len*scale);if(gw<2)gw=2;
    unsigned mix=(unsigned)(4+tv.grain[i].env*space*24);if(mix>28)mix=28;
    int a=gx,z=gx+gw;if(a<x0)a=x0;if(z>x0+w)z=x0+w;if(z>a)cv_rect(a,top,z-a,2*half+1,md_tint(MD_BG,dream_colors[i],mix));}
   for(int px=0;px<w;px++){
    unsigned behind=(unsigned)((w-1-px)/scale);
    if(behind>=tv.valid){if(px%3==0)cv_pset(x0+px,mid,MD_EDGE);continue;}
    unsigned idx=(tv.head-behind)&(tv.length-1);unsigned q=tv.peak[idx>>9]*half*half/255,h=0;while((h+1)*(h+1)<=q)h++; // square-root scale: quiet sends stay visible
    cv_rect(x0+px,mid-(int)h,1,2*(int)h+1,tv.freeze?md_tint(MD_BG,ice,22):MD_DIM);
   }
   for(i=0;i<4;i++){if(!tv.grain[i].active)continue;int gx=x0+w-1-(int)(tv.grain[i].behind*scale);
    if(gx>=x0)cv_rect(gx,top,1,2*half+1,md_tint(MD_BG,MD_INK,(unsigned)(8+tv.grain[i].env*24)));}
   cv_rect(x0+w-2,top-2,2,2*half+5,head_ink);
   if(tv.zone>=0){int zw=(int)(tv.zone_len*scale);if(zw<2)zw=2;int zx=x0+w-1-(int)(tv.zone*scale)-zw/2;
    cv_rect(zx<x0?x0:zx,top+2*half+3,zw,2,col);
    if(p->feedback){int fx=zx+zw/2;uint16_t fbk=md_tint(MD_BG,col,(unsigned)(4+p->feedback*24/1000));
     cv_rect(fx,top+2*half+7,x0+w-2-fx,1,fbk);cv_rect(x0+w-4,top+2*half+6,1,3,fbk);}}
   md_text(8,139,"-3 S",MD_DIM);md_text(84,139,tv.freeze?"FROZEN":p->reverse?"< REVERSE":"RECORDING",tv.freeze?ice:MD_INK);md_text(206,139,"NOW",MD_DIM);
   md_value(8,158,"K1 SPACE",p->macro[3]/10,col);md_value(124,158,"K2 GRAIN SIZE",p->grain_size/10,col);md_value(8,194,"K3 POSITION",p->grain_pos/10,col);md_value(124,194,"K4 FEEDBACK",p->feedback/10,col);
  } else if(dream_ui.page==ENVELOPE) {
   // The engine's own shape: linear attack (8 s x a^2), then exponential decay (tau = 0.06 + 16 s x d^2); the dot is the voice now.
   cv_rect(4,84,232,66,MD_BG);
   if(md_has_envelope(t->model)){
    const md_stats *ms=md_status();int tr=dream_ui.track;
    float att=8.0f*(t->attack*.001f)*(t->attack*.001f),tau=.06f+16.0f*(t->decay*.001f)*(t->decay*.001f),total=att+3*tau; /* down to -26 dB */
    cv_rect(30,133,180,1,MD_EDGE);
    for(i=0;i<180;i++){float tm=i*total/179,a=tm<att?tm/att:1/fm_expf((tm-att)/tau); /* fm_expf only with x >= 0: its 2^n step shifts n */cv_rect(30+i,133-(int)(a*45),1,2,col);}
    int ax=30+(int)(att/total*179);cv_rect(ax,131,1,5,MD_DIM);
    float e=ms->env_now[tr];
    if(e>.002f){float tm=ms->env_attacking[tr]?e*att:att+tau*fm_logf(1/e);if(tm>total)tm=total;md_circle(30+(int)(tm/total*179),133-(int)(e*45),3,MD_INK,1);}
    md_seconds(total,s);md_text(8,139,"0",MD_DIM);md_text(200-text_w(&FONT_XS,s),139,s,MD_DIM);
   } else md_text(24,139,"DRUMS / RINGS: OWN ENVELOPE",MD_DIM);
   if(md_has_envelope(t->model)){md_value_span(8,158,"K1 ATTACK ms",8*t->attack*t->attack/1000,0,8000,col);md_value_span(124,158,"K2 DECAY ms",60+16*t->decay*t->decay/1000,60,16060,col);}else{md_named(8,158,"K1 MELODIC ONLY","-",MD_DIM);md_value(124,158,"K2 DECAY",t->decay/10,col);}
   md_value(8,194,"K3 LEVEL",t->level/10,col);md_value(124,194,"K4 SEND",t->send/10,col);
  } else if(dream_ui.page==GROOVE) {
   // Steps of this track: odd steps shift right with Swing (dim ticks = straight grid), bar height = velocity
   // (Accent lifts step 1, lowers the others), sub-bars = Repeats, brightness = Chance, highlight = step playing.
   {const int x0=16,cw=13,base=130;int len=t->length>16?16:t->length,pos=-1;float sw=p->swing*.001f*.5f;
    cv_rect(4,84,232,66,MD_BG);
    if(md_playing())pos=(int)((md_status()->step/(unsigned)(t->speed>0?t->speed:1))%(unsigned)(t->length>0?t->length:1));
    unsigned mix=(unsigned)(10+t->prob*22/1000);
    for(i=0;i<len;i++){int gx=x0+i*cw;cv_rect(gx,base+2,1,3,MD_EDGE);
     int x=gx+(i%2?(int)(cw*sw+.5f):0);
     if(i==pos)cv_rect(x,88,cw-2,base-87,md_tint(MD_BG,col,6));
     if(!md_hit(i,t->hits,t->length,t->rotate)){cv_rect(x+3,base-1,cw-8,1,MD_EDGE);continue;}
     float vel=.8f+t->accent*.001f*(i==0?.2f:-.25f);int h=(int)(40*vel),r=t->ratchet<1?1:t->ratchet,sub=(cw-3)/r;
     for(int k=0;k<r;k++)cv_rect(x+1+k*sub,base-h,sub>2?sub-1:1,h,md_tint(MD_BG,col,mix));}
    char lab[24]="SWING ";md_num(p->swing/10,lab+6);char *e2=lab;while(*e2)e2++;*e2++='%';*e2++=' ';*e2++=' ';*e2++='/';md_num(t->speed,e2);
    md_text(x0,139,lab,MD_DIM);}
   md_value_span(8,158,"K1 SWING",p->swing/10,0,60,col);md_value_span(124,158,"K2 REPEATS",t->ratchet,1,4,col);
   md_value(8,194,"K3 ACCENT",t->accent/10,col);md_value_span(124,194,"K4 CLOCK DIV",t->speed,1,8,col);
  } else if(dream_ui.page==TAPE) {
   // Grain playback as an arrow (length = pitch ratio on a log scale, left = reverse) and the ducking now:
   // the source track and a bar of the gain reduction it applies to the other tracks.
   {cv_rect(4,84,232,66,MD_BG);const int cx=120,ay=98;cv_rect(24,ay,192,1,MD_EDGE);cv_rect(cx,ay-4,1,9,MD_DIM);
    int len=36+p->grain_pitch*3/2,dir=p->reverse?-1:1,tip;if(len<3)len=3;tip=cx+dir*len;
    cv_rect(dir>0?cx:tip,ay-1,len,3,col);for(int k=0;k<5;k++)cv_rect(tip-dir*k,ay-k,1,2*k+1,col);
    md_text(24,106,"SLOWER",MD_DIM);md_text(176,106,"FASTER",MD_DIM);
    md_text(12,124,"DUCK",MD_DIM);
    if(p->duck_source<0)md_text(52,124,"OFF",MD_EDGE);
    else{char n[4];md_num(p->duck_source+1,n);md_circle(58,130,7,dream_colors[p->duck_source],1);cv_text(56,124,&FONT_XS,n,MD_BG);
     float gr=1-md_status()->duck_gain;cv_rect(72,128,150,4,MD_CARD);cv_rect(72,128,(int)(gr/.9f*150),4,MD_HOT);}
    md_text(12,139,"GAIN REDUCTION ON OTHER TRACKS",MD_DIM);}
   md_value_span(8,158,"K1 PITCH st",p->grain_pitch,-24,24,col);md_named(124,158,"K2 DIRECTION",p->reverse?"REVERSE":"FORWARD",col);
   if(p->duck_source<0)md_named(8,194,"K3 DUCK FROM","OFF",col);else md_value_span(8,194,"K3 DUCK TRACK",p->duck_source+1,1,6,col);md_value_span(124,194,"K4 DUCK %",p->duck_amount*90/1000,0,90,col);
  } else {
   // Pan as a position between L and R, base note on a one-octave keyboard, clock division as pulses on 16 steps.
   {cv_rect(4,84,232,66,MD_BG);uint16_t c=t->mute?MD_EDGE:col;
    md_text(12,88,"L",MD_DIM);md_text(222,88,"R",MD_DIM);cv_rect(26,94,190,1,MD_EDGE);cv_rect(120,91,1,7,MD_EDGE);
    md_circle(26+t->pan*190/1000,94,5,c,1);
    char nn[8];int pc=t->note%12;const char *nm=md_pitch_names[pc];int k2=0;while(nm[k2]){nn[k2]=nm[k2];k2++;}md_num(t->note/12-1,nn+k2);
    cv_text(14,106,&FONT_B,nn,c);
    static const int wx[7]={0,2,4,5,7,9,11};
    for(i=0;i<7;i++)cv_rect(84+i*20,106,19,22,wx[i]==pc?c:MD_DIM);
    static const int bx[5]={1,3,6,8,10},bp[5]={0,1,3,4,5};
    for(i=0;i<5;i++)cv_rect(84+bp[i]*20+13,106,12,13,bx[i]==pc?c:MD_BG);
    int pos=md_playing()?(int)(md_status()->step%16):-1;
    for(i=0;i<16;i++){int on=i%(t->speed>0?t->speed:1)==0;cv_rect(84+i*9,134,on?5:3,on?4:2,i==pos?MD_INK:on?c:MD_EDGE);}
    md_text(12,132,t->mute?"MUTED":"CLOCK",t->mute?MD_HOT:MD_DIM);}
   md_value(8,158,"K1 SEND",t->send/10,col);md_value_span(124,158,"K2 MIDI NOTE",t->note,24,96,col);md_value(8,194,"K3 PAN",t->pan/10,col);md_value_span(124,194,"K4 CLOCK DIV",t->speed,1,8,col);
  }
 }
 if(plat_ms()<dream_ui.msg_until){cv_rect(0,224,240,16,col);md_text(6,224,dream_ui.msg,MD_BG);}
 else {
  if(dream_ui.page==LIVE){md_text(8,226,monde_label(p->biome_id),MD_INK);if(p->edited_mask&1)md_text(151,226,"*",col);md_text(164,226,p->locked?"LOCK":"RAND",MD_DIM);md_text(203,226,p->freeze?"ICE":"REC",MD_DIM);}
  else if(dream_ui.page==MORPH)md_text(10,226,"PRESET: MONDE B  /  HOLD: HELP",MD_DIM);
  else if(dream_ui.page==DRIFT)md_text(10,226,"REC: NEW HOME  /  ENV: HOME",MD_DIM);
  else md_text(10,226,(dream_ui.page==SOUND||dream_ui.page==FILTER||dream_ui.page==ENVELOPE||dream_ui.page==MIX||dream_ui.page==MIXER||dream_ui.page==BASSLINE||dream_ui.page==MODULATION||dream_ui.page==PHRASE||dream_ui.page==LFO_A||dream_ui.page==LFO_B||dream_ui.page==GENERATOR)?"PRESET: TONE  /  HOLD: HELP":"PRESET: MONDE  /  HOLD: HELP",MD_DIM);
 }
}
void app_say(const char *a,const char *b) {unsigned i=0;(void)b;while(a&&a[i]&&i<31){dream_ui.msg[i]=a[i];i++;}dream_ui.msg[i]=0;dream_ui.msg_until=plat_ms()+1400;dream_ui.dirty=1;ann_say(dream_ui.msg,0,ANN_NOW);}
void app_init(void) {
 md_patch saved;md_init();midi_link=plat_midi_out_link();overloads=cpu_peak=quality_notice=0;
 dream_prefs.magic=0x4D445046;dream_prefs.motion=1;dream_prefs.boot_sound=1;dream_midi_defaults(&dream_prefs.midi);
 dream_prefs.custom_scale=0x091;
 {static struct {uint32_t magic;int32_t motion,boot_sound;md_midi_routing midi;int32_t custom_scale;} prefs;memset(&prefs,0,sizeof prefs);
  int got=plat_store_load(OBJ_PREFS,&prefs,sizeof prefs);
  if((got==12||got==(int)sizeof prefs)&&prefs.magic==0x4D445046&&prefs.motion>=0&&prefs.motion<=1&&prefs.boot_sound>=0&&prefs.boot_sound<=1){
   dream_prefs.motion=prefs.motion;dream_prefs.boot_sound=prefs.boot_sound;if(got==(int)sizeof prefs&&dream_midi_ok(&prefs.midi)&&prefs.custom_scale>=0&&prefs.custom_scale<=0xfff){dream_prefs.midi=prefs.midi;dream_prefs.custom_scale=prefs.custom_scale;}}}
 md_set_midi_routing(&dream_prefs.midi);md_set_custom_scale((unsigned)dream_prefs.custom_scale);
 intro_pending=1;intro_until=chime_left=0;md_gain=0;memset(&dream_ui,0,sizeof dream_ui);dream_ui.help=-1;
 int bytes=plat_store_load(OBJ_PATCH,&saved,sizeof saved);if(bytes>0)md_import(&saved,(unsigned)bytes);
}
// Generated notes and clock to USB (and Bluetooth when built in). A note-on only goes when the port keeps room for
// its note-off; real-time and note-offs always try.
static void dream_send_midi(void){
 uint8_t m[3];
 while(md_midi_take(m)){
  uint32_t cin=m[0]>=0xf8?0xfu:(uint32_t)(m[0]>>4);
  if((m[0]&0xf0)==0x90&&plat_midi_out_room(TO_USB|TO_BLE)<8)continue;
  plat_midi_send(cin|((uint32_t)m[0]<<8)|((uint32_t)m[1]<<16)|((uint32_t)m[2]<<24),TO_USB|TO_BLE);
 }
}
void app_render(int32_t *lr,uint32_t n,uint32_t master) {
 if(dream_reloading){memset(lr,0,n*2*sizeof *lr);return;}
 uint32_t pkt,load=plat_cpu_pct();int port;if(load>cpu_peak)cpu_peak=load;
 uint32_t link=plat_midi_out_link();if(link!=midi_link){midi_link=link;for(unsigned i=0;i<6;i++)md_midi(0xb0+i,123,0,0);}
 for(unsigned m=0;m<16&&plat_midi_in(&pkt,&port);m++)md_midi((pkt>>8)&255,(pkt>>16)&127,(pkt>>24)&127,port);
 while(n){unsigned k,z=n>128?128:n;md_render(md_audio,z);dream_send_midi();if(md_playing())chime_left=0;for(k=0;k<z;k++){md_gain+=((float)master/4096.0f-md_gain)*.004f;float chime=0;

 if(chime_left){unsigned elapsed=88200-chime_left;float env=elapsed<11025?elapsed/11025.0f:chime_left/77175.0f;const float hz[3]={261.6256f,391.9954f,523.2511f};for(int c=0;c<3;c++){chime_phase[c]+=hz[c]/44100.0f;if(chime_phase[c]>=1)chime_phase[c]-=1;chime+=fm_sinf(chime_phase[c]*6.2831853f)*.009f*env;}chime_left--;}
 lr[2*k]=(int32_t)((md_audio[2*k]+chime)*md_gain*8380000.0f);lr[2*k+1]=(int32_t)((md_audio[2*k+1]+chime)*md_gain*8380000.0f);}lr+=z*2;n-=z;}
}
void app_panic(void) {dream_cmd(MD_PANIC,0,0);}
void app_overload(void) {overloads++;dream_cmd(MD_QUALITY,0,0);}
// Lights say what is in use: the button of the page shown, SEL when the selected track is muted, ARP locked,
// LFO frozen, PLAY playing (green); ENV glows when there is something to undo.
// The five page buttons and their pages, in the order each press goes through them.
static const struct { int button; uint8_t pages[8]; int n; } page_cycles[5]={
 {B_HOME,{LIVE,MORPH,DRIFT},3},{B_EDIT,{SOUND,FILTER,ENVELOPE,MIX,MIXER},5},{B_SEQ,{RHYTHM,GROOVE,BASSLINE,PHRASE,GENERATOR,FORM},6},
 {B_FX,{GRAIN,TAPE,SPACE,SPACE_TONE,ECHO_CHORUS,MODULATION,LFO_A,LFO_B},8},{B_GLO,{HARMONY,RAGA,SYSTEM,APPEARANCE,MIDI,ZONES,BANKS},7}};
static int page_button(int page){for(int c=0;c<5;c++)for(int k=0;k<page_cycles[c].n;k++)if(page_cycles[c].pages[k]==page)return page_cycles[c].button;return B_HOME;}
// A page button: from one of its pages to the next (the last goes back to the first), from anywhere else to its first page.
static int page_next(int button,int page){for(int c=0;c<5;c++)if(page_cycles[c].button==button){
  for(int k=0;k<page_cycles[c].n;k++)if(page_cycles[c].pages[k]==page)return page_cycles[c].pages[(k+1)%page_cycles[c].n];return page_cycles[c].pages[0];}return page;}
static uint32_t dream_lights(uint32_t *dim){
 const md_patch *lp=md_state();const int page_btn=page_button(dream_ui.page);
 if(dim)*dim=md_can_return()?1u<<B_ENV:0;
 return (1u<<page_btn)|(md_playing()?LED_PLAY_GREEN:0)|(lp->locked?1u<<B_ARP:0)|(lp->freeze?1u<<B_LFO:0)|(lp->track[dream_ui.track].mute?1u<<B_SEL:0);
}
// MORPH: K1 or PRESET picks Monde B (the morph starts from the patch as it is); REC keeps the mix.
static void morph_choose(int d){md_morph_state ms;md_morph_view(&ms,0,0);int id=clamp((ms.on?ms.target:md_state()->biome_id)+d,0,MD_BIOMES-1);
 static char m[40];const char *a="MORPH TO ",*z=monde_label(id)+6;int k=0;while(*a)m[k++]=*a++;while(*z&&k<39)m[k++]=*z++;m[k]=0;app_say(dream_cmd(MD_MORPH_TO,0,id)?m:"BUSY: TRY AGAIN","");}
static void md_tap(int b) {
 const md_patch *p=md_state();switch(b){
 case B_HOME:dream_ui.page=dream_ui.page==LIVE&&plat_ms()-intro_left_at<=1500?LIVE:page_next(B_HOME,dream_ui.page);break; // the tap that ends the intro stays on LIVE
 case B_EDIT:case B_SEQ:case B_FX:case B_GLO:dream_ui.page=page_next(b,dream_ui.page);break;
 case B_SEL:dream_cmd(MD_MUTE,dream_ui.track,!p->track[dream_ui.track].mute);break;
 case B_PLAY:dream_cmd(MD_PLAY,0,!md_playing());break;case B_ARP:dream_cmd(MD_LOCK,0,!p->locked);app_say(p->locked?"ARP: SEED FREE, VARIES":"ARP: SEED LOCKED, REPEATS","");break;
 // The printed labels (ENV, LFO, ARP, REC) come from the stock firmware: each press says what it did.
 case B_REC:if(dream_ui.page==MORPH){md_drift_state dv;md_drift_view(&dv,0);if(dv.on){app_say("DERIVE IS ON: HOME FOR DERIVE","");break;}}
  if(dream_ui.page==DRIFT){md_drift_state ds;md_drift_view(&ds,0);if(!ds.on){app_say("K2: HOW SLOW, THEN PLAY","");break;}dream_cmd(MD_DRIFT_HOME,0,0);app_say("NEW HOME: HERE","");break;}
  if(dream_ui.page==MORPH){md_morph_state ms;md_morph_view(&ms,0,0);if(!ms.on){app_say("K1: CHOOSE MONDE B","");break;}
  dream_cmd(MD_MORPH_KEEP,0,0);app_say(ms.pos==1000&&!ms.scope?"KEPT: NOW IN MONDE B":"KEPT: THE MIX STAYS","");break;}
  if(dream_ui.page==GENERATOR){app_say(dream_cmd(MD_CLEAR_PHRASE,dream_ui.track,0)?"MOTIF CLEARED":"BUSY: TRY AGAIN","");motif_at=-1;break;} /* one command: all or nothing */
 {int pg=dream_ui.page,panic=pg==SYSTEM||pg==MIDI;
  // Dice: REC rolls the settings of the page shown (its K1-K4); ENV undoes it. LIVE and the GLO pages keep the new variation.
  static const int dice_pages[][2]={{SOUND,MD_DICE_SOUND},{FILTER,MD_DICE_FILTER},{ENVELOPE,MD_DICE_ENVELOPE},{MIX,MD_DICE_MIX},{MIXER,MD_DICE_MIXER},
   {RHYTHM,MD_DICE_RHYTHM},{GROOVE,MD_DICE_GROOVE},{BASSLINE,MD_DICE_BASS},{PHRASE,MD_DICE_PHRASE},{FORM,MD_DICE_FORM},{GRAIN,MD_DICE_GRAIN},{TAPE,MD_DICE_TAPE},
   {SPACE,MD_DICE_SPACE},{SPACE_TONE,MD_DICE_SPACE_TONE},{ECHO_CHORUS,MD_DICE_ECHO},{MODULATION,MD_DICE_MODULATION},{LFO_A,MD_DICE_LFO_A},{LFO_B,MD_DICE_LFO_B},{RAGA,MD_DICE_RAGA}};
  static const char *const dice_names[MD_DICE_PAGES]={"SOUND","FILTER","ENVELOPE","MIX","MIXER","RHYTHM","GROOVE","BASS","PHRASE","FORM","GRAIN","TAPE","SPACE","SPACE TONE","ECHO / CHORUS","MODULATION","LFO A","LFO B","RAGA"};
  int scope=-1;for(unsigned k=0;k<sizeof dice_pages/sizeof dice_pages[0];k++)if(dice_pages[k][0]==pg)scope=dice_pages[k][1];
  if(scope>=0){static char m[32];const char *f="DICE: ";int k=0;while(*f)m[k++]=*f++;for(f=dice_names[scope];*f;)m[k++]=*f++;
   const int shared=scope==MD_DICE_FORM||(scope>=MD_DICE_GRAIN&&scope<=MD_DICE_ECHO); // these pages are not a track's
   if(!shared){m[k++]=' ';m[k++]='T';m[k++]='1'+dream_ui.track;}m[k]=0;app_say(dream_cmd(MD_DICE,dream_ui.track,scope)?m:"BUSY: TRY AGAIN","");}
  else {dream_cmd(panic?MD_PANIC:MD_MUTATE,0,0);app_say(panic?"PANIC: ALL SILENT":"REC: NEW VARIATION","");}}break;
 case B_ENV:if(md_can_return()){dream_cmd(MD_RETURN,0,0);app_say("ENV: PREVIOUS VARIATION","");}else app_say("ENV: NOTHING TO UNDO YET","");break;
 case B_LFO:dream_cmd(MD_FREEZE,0,!p->freeze);app_say(p->freeze?"LFO: GRAIN TAPE RUNS":"LFO: GRAIN TAPE FROZEN","");break;
 case B_OCTDN:dream_ui.octave=clamp(dream_ui.octave-1,-2,2);break;case B_OCTUP:dream_ui.octave=clamp(dream_ui.octave+1,-2,2);break;
 case B_SAVE:if(dream_store_hold){app_say("USB BACKUP IN PROGRESS","");break;}if(dream_ui.page==BANKS){bank_action(1);break;}if(md_playing())app_say("STOP BEFORE SAVING","");else if(md_clock_source())app_say("USE INTERNAL CLOCK TO SAVE","");else {md_patch snapshot;uint32_t f=plat_lock();snapshot=*p;plat_unlock(f);int failed=plat_store_save(OBJ_PATCH,&snapshot,sizeof snapshot);if(!failed)failed=plat_store_save(OBJ_PREFS,&dream_prefs,sizeof dream_prefs);app_say(failed?"SAVE FAILED":"MONDE + PREFS SAVED","");}break;
 }
}
void app_input_only(void) {
 unsigned b,now=plat_ms(),btn=plat_buttons(),keys=plat_keys();const md_patch *p=md_state();int d;
 for(b=0;b<NB;b++) {
  if((btn>>b&1)&&!(dream_ui.btn>>b&1)){dream_ui.down[b]=now;dream_dismiss_intro();}
  if((btn>>b&1)&&!(dream_ui.held>>b&1)&&now-dream_ui.down[b]>=850){dream_ui.held|=1u<<b;dream_ui.help=b;ann_say(help_title[b],0,ANN_NOW);}
  if(!(btn>>b&1)&&(dream_ui.btn>>b&1)){if(!(dream_ui.held>>b&1)){if(dream_ui.help>=0)dream_ui.help=-1;else md_tap(b);}dream_ui.held&=~(1u<<b);}
 }
 dream_ui.btn=btn;
 if((d=plat_enc(EN_SELECT))){dream_dismiss_intro();dream_ui.track=clamp(dream_ui.track+d,0,5);}
 if((d=plat_enc(EN_ALGO))){dream_dismiss_intro();dream_cmd(MD_MODEL,dream_ui.track,clamp(p->track[dream_ui.track].model+d,0,MD_MODELS-1));dream_ui.page=SOUND;}
 if((d=plat_enc(EN_PRESET))&&dream_ui.help<0){dream_dismiss_intro();
  if(dream_ui.page==BANKS){for(int j=0;j<=bank_char;j++)if(!bank_name[j])bank_name[j]=' ';bank_name[bank_char]=clamp(bank_name[bank_char]+d,32,126);bank_name[23]=0;bank_armed=-1;}else if(dream_ui.page==SOUND||dream_ui.page==FILTER||dream_ui.page==ENVELOPE||dream_ui.page==MIX||dream_ui.page==MIXER||dream_ui.page==BASSLINE||dream_ui.page==MODULATION||dream_ui.page==PHRASE||dream_ui.page==LFO_A||dream_ui.page==LFO_B||dream_ui.page==GENERATOR){
   int id=p->tone_preset[dream_ui.track];if(id<0)id=p->track[dream_ui.track].model*16-(d>0?1:0);
   id=clamp(id+d,0,MD_TONE_PRESETS-1);dream_cmd(MD_TONE_PRESET,dream_ui.track,id);app_say(md_tone_name(id),"");
  }else if(dream_ui.page==MORPH)morph_choose(d);
  else{int id=clamp(p->biome_id+d,0,MD_BIOMES-1);dream_cmd(MD_SCENE,0,id);app_say(monde_label(id),"");}
 }
 for(b=0;b<4;b++)if((d=plat_enc(EN_K1+b))&&dream_ui.help<0) {
  dream_dismiss_intro();const md_track *t=&p->track[dream_ui.track];int c=0,v=0,tr=dream_ui.track;
  if(dream_ui.page==DRIFT){md_drift_state ds;md_drift_view(&ds,0);const int cs[4]={MD_DRIFT_AMOUNT,MD_DRIFT_SLOW,MD_DRIFT_VOYAGE,MD_DRIFT_SCOPE},vs[4]={ds.amount,ds.slow,ds.voyage,ds.scope},hi[4]={2,6,2,MD_MORPH_SCOPES-1};
   dream_cmd(cs[b],0,clamp(vs[b]+d,0,hi[b]));continue;}
  if(dream_ui.page==MORPH){md_morph_state ms;md_morph_view(&ms,0,0);md_drift_state dv;md_drift_view(&dv,0);
   if(dv.on&&b){app_say("DERIVE IS ON: HOME FOR DERIVE","");continue;} // K1 / PRESET still pick a Monde B, which stops the drift
   if(b==0)morph_choose(d);else if(b==1){if(ms.on)dream_cmd(MD_MORPH,0,clamp(ms.pos+d*20,0,1000));else app_say("K1: CHOOSE MONDE B","");}
   else if(b==2)dream_cmd(MD_MORPH_BARS,0,clamp(ms.bars+d,0,5));else dream_cmd(MD_MORPH_SCOPE,0,clamp(ms.scope+d,0,MD_MORPH_SCOPES-1));continue;}
  if(dream_ui.page==BANKS){if(b==0){bank_slot=clamp(bank_slot+d,0,bank_kind?63:15);bank_armed=-1;}else if(b==1){bank_kind=clamp(bank_kind+d,0,1);bank_slot=clamp(bank_slot,0,bank_kind?63:15);bank_armed=-1;}else if(b==2&&d>0)bank_action(0);else if(b==3)bank_char=clamp(bank_char+d,0,22);continue;}
  if(dream_ui.page==MIDI){if(b==0)dream_cmd(MD_CLOCK_SOURCE,0,md_clock_source()+d);else if(b==1)dream_cmd(MD_MIDI_QUANTIZE,0,md_midi_quantize()+d);
   else{if(b==2)dream_prefs.midi.mode=clamp(dream_prefs.midi.mode+d,0,2);else dream_prefs.midi.clock=clamp(dream_prefs.midi.clock+d,0,1);dream_apply_midi();}continue;}
  if(dream_ui.page==ZONES){
   if(b==0)zone_sel=clamp(zone_sel+d,0,7);
   else{md_midi_routing *r=&dream_prefs.midi;int *v=b==1?&r->zone[zone_sel].lo:b==2?&r->zone[zone_sel].hi:&r->zone[zone_sel].ch;
    *v=b==3?clamp(*v+d,0,16):clamp(*v+d,24,96);if(r->zone[zone_sel].lo>r->zone[zone_sel].hi){if(b==1)r->zone[zone_sel].hi=r->zone[zone_sel].lo;else r->zone[zone_sel].lo=r->zone[zone_sel].hi;}
    dream_apply_midi();}
   continue;}
  if(dream_ui.page==APPEARANCE){if(b==0)dream_prefs.motion=clamp(dream_prefs.motion+d,0,1);else if(b==1)dream_prefs.boot_sound=clamp(dream_prefs.boot_sound+d,0,1);else if(b==2)dream_intro();else{cpu_peak=0;plat_audio_peak_reset();}continue;}
  if(dream_ui.page==LIVE){c=MD_MACRO;tr=b;v=p->macro[b]+d*20;}
  else if(dream_ui.page>=MIXER){const md_extension *e=&p->ext[tr];
   if(dream_ui.page==MIXER){const int cs[]={MD_LEVEL,MD_SEND,MD_DELAY_SEND,MD_REVERB_SEND},vs[]={t->level,t->send,e->delay_send,e->reverb_send};c=cs[b];v=vs[b]+d*20;}
   else if(dream_ui.page==BASSLINE){const int cs[]={MD_BASS_MODE,MD_BASS_PATTERN,MD_GLIDE,MD_NOTE},vs[]={e->bass_mode,e->pattern,e->glide,t->note};c=cs[b];v=vs[b]+d*(b==2?20:1);}
   else if(dream_ui.page==SPACE){const int cs[]={MD_DELAY_TIME,MD_DELAY_FB,MD_REVERB_TIME,MD_FX_LEVEL},vs[]={p->delay_ms,p->delay_fb,p->reverb_time,p->fx_level};c=cs[b];v=vs[b]+d*(b==0?5:20);
    if(b==0&&p->echo_sync){c=MD_ECHO_SYNC;v=clamp(p->echo_sync+d,1,MD_ECHO_SYNCS-1);}} /* synced: K1 picks the division */
   else if(dream_ui.page==MODULATION){const int cs[]={MD_MARBLES,MD_TIDES,MD_TIDES_RATE,MD_WARPS},vs[]={e->marbles,e->tides,e->tides_rate,e->warps};c=cs[b];v=vs[b]+d*20;}
   else if(dream_ui.page==LFO_A||dream_ui.page==LFO_B){const md_lfo *l=&p->lfo[tr][dream_ui.page==LFO_B];const int cs[]={MD_LFO_RATE,MD_LFO_SHAPE,MD_LFO_TARGET,MD_LFO_DEPTH},vs[]={l->rate,l->shape,l->target,l->depth};c=cs[b];v=vs[b]+d*(b==3?10:1);tr+=dream_ui.page==LFO_B?6:0;}
   else if(dream_ui.page==RAGA){const int cs[]={MD_VADI,MD_SAMVADI,MD_GAMAKA,MD_GAMAKA_AMOUNT},vs[]={p->vadi,p->samvadi,p->gamaka[tr],p->gamaka_amount[tr]};c=cs[b];v=vs[b]+d*(b==3?20:1);
    if(b<2){unsigned mask=md_scale_mask(p->scale);v=vs[b];int step=d>0?1:-1;for(int n=d>0?d:-d;n>0;n--){do v+=step;while(v>=0&&v<=11&&!((mask>>v)&1));if(v<-1)v=-1;if(v>11){v=vs[b];break;}}}} /* pivots are scale notes */
   else if(dream_ui.page==GENERATOR){const md_gen *g=&p->gen[tr];const int cs[]={MD_GEN_SOURCE,MD_GEN_DEJAVU,MD_GEN_LOOP,MD_GEN_SPREAD},vs[]={g->source,g->dejavu,g->loop,g->spread};c=cs[b];v=vs[b]+d*(b==1?20:1);}
   else if(dream_ui.page==FORM){const int cs[]={MD_FORM,MD_FORM_BARS,MD_FORM_DROPS,MD_MOTION_EVERY},vs[]={p->form,p->form_bars,p->form_drops,p->motion_every};c=cs[b];v=vs[b]+d*(b==0?20:1);}
   else if(dream_ui.page==SPACE_TONE){const int cs[]={MD_SEND_HP,MD_ECHO_TONE,MD_ROOM_TONE,MD_ROOM_DIFFUSION},vs[]={p->send_hp,p->echo_tone,p->room_tone,p->room_diffusion};c=cs[b];v=vs[b]+d*20;}
   else if(dream_ui.page==ECHO_CHORUS){const int cs[]={MD_ECHO_SYNC,MD_DELAY_FB,MD_CHORUS,MD_CHORUS_DEPTH},vs[]={p->echo_sync,p->delay_fb,p->chorus,p->chorus_depth};c=cs[b];v=vs[b]+d*(b==0?1:20);}
   else{const int cs[]={MD_BASS_STEP,MD_BASS_VALUE,MD_ACCENT,MD_PROB},vs[]={e->edit_step,e->notes[e->edit_step],t->accent,t->prob};c=cs[b];v=vs[b]+d*(b<2?1:20);}
  }
  else if(dream_ui.page==SOUND){const int cmds[]={MD_TONE,MD_HARM,(md_has_envelope(t->model)||t->model==MD_ELEMENTS)?MD_SHAPE:MD_DECAY,MD_LEVEL};const int vals[]={t->tone,t->harm,(md_has_envelope(t->model)||t->model==MD_ELEMENTS)?t->shape:t->decay,t->level};c=cmds[b];v=vals[b]+d*20;}
  else if(dream_ui.page==FILTER){const int cmds[]={MD_CUTOFF,MD_RESONANCE,MD_FILTER,MD_BREATHE};const int vals[]={t->cutoff,t->resonance,t->filter,t->breathe};c=cmds[b];v=vals[b]+d*(b==2?1:20);}
  else if(dream_ui.page==RHYTHM){const int cmds[]={MD_HITS,MD_LENGTH,MD_ROTATE,MD_PROB};const int vals[]={t->hits,t->length,t->rotate,t->prob};c=cmds[b];v=vals[b]+d*(b==3?20:1);}
  else if(dream_ui.page==GRAIN){const int cmds[]={MD_MACRO,MD_GRAIN_SIZE,MD_GRAIN_POS,MD_FEEDBACK};const int vals[]={p->macro[3],p->grain_size,p->grain_pos,p->feedback};c=cmds[b];v=vals[b]+d*20;tr=3;}
  else if(dream_ui.page==MIX){const int cmds[]={MD_SEND,MD_NOTE,MD_PAN,MD_SPEED};const int vals[]={t->send,t->note,t->pan,t->speed};c=cmds[b];v=vals[b]+d*((b==0||b==2)?20:1);}
  else if(dream_ui.page==ENVELOPE){const int cmds[]={MD_ATTACK,MD_DECAY,MD_LEVEL,MD_SEND};const int vals[]={t->attack,t->decay,t->level,t->send};c=cmds[b];v=vals[b]+d*20;}
  else if(dream_ui.page==GROOVE){const int cmds[]={MD_SWING,MD_RATCHET,MD_ACCENT,MD_SPEED};const int vals[]={p->swing,t->ratchet,t->accent,t->speed};c=cmds[b];v=vals[b]+d*((b==0||b==2)?20:1);}
  else if(dream_ui.page==TAPE){const int cmds[]={MD_GRAIN_PITCH,MD_REVERSE,MD_DUCK_SOURCE,MD_DUCK};const int vals[]={p->grain_pitch,p->reverse,p->duck_source,p->duck_amount};c=cmds[b];v=vals[b]+d*(b==3?20:1);}
  else if(dream_ui.page==HARMONY&&b==1){c=MD_SCALE;v=md_scale_at(clamp(md_scale_position(p->scale)+d,0,md_scale_count()-1));d=0;}
  else if(dream_ui.page==HARMONY){const int cmds[]={MD_ROOT,MD_SCALE,MD_TRANSPOSE,MD_TEMPO};const int vals[]={p->root,p->scale,p->transpose,p->tempo};c=cmds[b];v=vals[b]+d;}
  else {if(b==3){cpu_peak=0;plat_audio_peak_reset();continue;}const int cmds[]={MD_TEMPO,MD_QUALITY,MD_DUCK_RELEASE};const int vals[]={p->tempo,p->quality,p->duck_release};c=cmds[b];v=vals[b]+d*(b==2?20:1);}
  dream_cmd(c,tr,v);
 }
 for(b=0;b<27;b++)if((keys>>b&1)&&!(dream_ui.keys>>b&1)){dream_dismiss_intro();
  // On HARMONY with the CUSTOM scale, a key toggles its note in the scale instead of playing (the root stays).
  if(dream_ui.help<0&&dream_ui.page==HARMONY&&p->scale==md_scale_custom()){int iv=((53+b)%12-p->root+12)%12;if(iv){dream_prefs.custom_scale^=1<<iv;uint32_t f=plat_lock();md_set_custom_scale((unsigned)dream_prefs.custom_scale);plat_unlock(f);}}
  // On GENERATOR, a key also writes its note into the motif (folded into the 0..+24 range above the base) and moves on.
  else if(dream_ui.help<0&&dream_ui.page==GENERATOR){int tr=dream_ui.track,key=53+b+dream_ui.octave*12,off=key-p->track[tr].note;while(off<0)off+=12;while(off>24)off-=12;
   // The engine applies queued commands later: keys pressed within one frame continue from the UI's own cursor.
   int at=(motif_at>=0&&motif_track==tr&&plat_ms()<motif_until)?motif_at:p->ext[tr].edit_step;motif_at=(at+1)%16;motif_track=tr;motif_until=plat_ms()+400;
   dream_cmd(MD_BASS_STEP,tr,at);dream_cmd(MD_BASS_VALUE,tr,off);dream_cmd(MD_BASS_STEP,tr,(at+1)%16);dream_cmd(MD_TRIGGER,tr,key);}
  else if(dream_ui.help<0)dream_cmd(MD_TRIGGER,dream_ui.track,53+b+dream_ui.octave*12);}
 dream_ui.keys=keys;
 /* The printed SEL key mutes; EDIT cycles sound, envelope, mix, avoiding a hidden modifier. */
}
void app_ui_init(void) {dream_ui.dirty=1;dream_ui.next=0;if(intro_pending){intro_pending=0;dream_intro();}}
void app_frame(void) {
 unsigned y;app_input_only();if(md_status()->quality_reductions!=quality_notice){quality_notice=md_status()->quality_reductions;app_say("ECO: DENSE PATCH","");}flower_time=dream_prefs.motion?plat_ms():0;for(int i=0;i<6;i++)if(md_status()->events[i]!=last_events[i]){last_events[i]=md_status()->events[i];pulse_until[i]=plat_ms()+160;}if(plat_ms()<dream_ui.next&&!dream_ui.dirty)return;dream_ui.next=plat_ms()+80;dream_ui.dirty=0;
 for(y=0;y<240;y+=40){cv_begin(240,40,MD_BG);cv_oy=-(int)y;md_draw_all();cv_blit(0,y);}
 {uint32_t dim;const uint32_t lit=dream_lights(&dim);plat_lights(lit,dream_ui.keys,dim,0,1);}
 ann_tick(plat_ms());
}
#ifdef PLAT_HOST
int app_expect(const char *what,int *got) {
 if(!strcmp(what,"help_context")){*got=md_context_help();return 1;}
 if(!strcmp(what,"lights")){*got=(int)(dream_lights(0)&((1u<<NB)-1));return 1;}
 if(!strcmp(what,"glow")){uint32_t d;dream_lights(&d);*got=(int)d;return 1;}
 if(!strncmp(what,"drift_",6)){md_drift_state ds;md_drift_view(&ds,0);const char *w=what+6;*got=!strcmp(w,"on")?ds.on:!strcmp(w,"slow")?ds.slow:!strcmp(w,"amount")?ds.amount:!strcmp(w,"voyage")?ds.voyage:ds.scope;return 1;}
 if(!strncmp(what,"morph_",6)){md_morph_state ms;md_morph_view(&ms,0,0);const char *w=what+6;*got=!strcmp(w,"on")?ms.on:!strcmp(w,"target")?ms.target:!strcmp(w,"pos")?ms.pos:!strcmp(w,"bars")?ms.bars:ms.scope;return 1;}
 if(!strcmp(what,"motion"))*got=dream_prefs.motion;else if(!strcmp(what,"boot_sound"))*got=dream_prefs.boot_sound;else if(!strcmp(what,"intro"))*got=intro_until&&plat_ms()<intro_until;else if(!strcmp(what,"track"))*got=dream_ui.track;else if(!strcmp(what,"model"))*got=md_state()->track[dream_ui.track].model;else if(!strcmp(what,"help"))*got=dream_ui.help;
 else if(!strcmp(what,"biome"))*got=md_state()->biome_id;else if(!strcmp(what,"tone_preset"))*got=md_state()->tone_preset[dream_ui.track];
 else if(!strcmp(what,"page"))*got=dream_ui.page;else if(!strcmp(what,"playing"))*got=md_playing();else if(!strcmp(what,"locked"))*got=md_state()->locked;
 else if(!strcmp(what,"density"))*got=md_state()->macro[0];else if(!strcmp(what,"freeze"))*got=md_state()->freeze;else if(!strcmp(what,"quality"))*got=md_state()->quality;
 else if(!strcmp(what,"scale"))*got=md_state()->scale;else if(!strcmp(what,"attack"))*got=md_state()->track[dream_ui.track].attack;else if(!strcmp(what,"ratchet"))*got=md_state()->track[dream_ui.track].ratchet;
 else if(!strncmp(what,"motif",5)&&what[5]>='0'&&what[5]<='9')*got=md_state()->ext[dream_ui.track].notes[atoi(what+5)&15];
 else if(!strcmp(what,"invalid"))*got=md_status()->nonfinite;else if(!strcmp(what,"queue_drops"))*got=md_status()->dropped_commands;else return 0;return 1;
}

#endif

#include "transfer.c"
#include "web.c"
#include "diagnostics.c"

void app_splash(void){for(unsigned y=0;y<240;y+=40){cv_begin(240,40,MD_BG);cv_oy=-(int)y;
 unsigned r=0x2545f491u;for(int k=0;k<70;k++){r^=r<<13;r^=r>>17;r^=r<<5;cv_pset((int)(r%240),(int)((r>>8)%240),k%3?MD_EDGE:MD_DIM);}
 md_circle(120,96,4,MD_INK,1);for(int j=0;j<6;j++){float a=j*1.04719755f-.52f;md_ship(120+fm_cosf(a)*30,96+fm_sinf(a)*30,a+1.5707963f,7,dream_colors[j],1,3,dream_colors[j]);}
 cv_text(120-text_w(&FONT_B,"MONADES")/2,160,&FONT_B,"MONADES",MD_INK);md_text(120-text_w(&FONT_XS,"FLASH DOG DUKE SILVER")/2,186,"FLASH DOG DUKE SILVER",MD_DIM);cv_blit(0,y);}}
