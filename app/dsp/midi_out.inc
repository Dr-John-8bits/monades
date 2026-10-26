/* SPDX-License-Identifier: MIT OR GPL-3.0-only
 * Monades: MIDI out (queue, owed note-offs, clock, zones, pitch bend). Included by dreams.cpp, inside its anonymous namespace. */
// Monades MIDI out: a queue drained by the app, the note-offs owed, and the clock.
struct OutMsg { uint8_t st,d1,d2,pad; };
static OutMsg outq[256];static volatile unsigned out_r,out_w;
struct OwedOff { float left; uint8_t ch,note; };
static OwedOff owed[64];static unsigned owed_n;
static md_midi_routing routing;static float clock_left;
static void out_push(uint8_t st,uint8_t d1,uint8_t d2){unsigned w=out_w;if(w-out_r>=256){stats.midi_out_dropped++;return;}outq[w&255]={st,d1,d2,0};fence();out_w=w+1;}
static int chan_bend[16]; /* pitch bend sent per channel, -8192..8191 (range assumed +/-2 semitones) */
static void out_bend(unsigned ch,int bend){if(bend==chan_bend[ch])return;chan_bend[ch]=bend;unsigned v=(unsigned)(bend+8192);out_push(0xe0|ch,v&127,(v>>7)&127);}
static void out_all_off(){for(unsigned j=0;j<owed_n;j++)out_push(0x80|owed[j].ch,owed[j].note,0);owed_n=0;}
static void out_note(unsigned track,float note,float velocity,float gate){
 if(!routing.mode)return;
 int n=bound((int)(note+.5f),0,127),vel=bound((int)(velocity*127+.5f),1,127);unsigned chans=0;
 if(routing.mode==1)chans=1u<<track;
 else for(unsigned z=0;z<8;z++){const int ch=routing.zone[z].ch;if(ch>=1&&ch<=16&&n>=routing.zone[z].lo&&n<=routing.zone[z].hi)chans|=1u<<(ch-1);}
 // Per-track mode: one voice per channel, so a microtonal note goes out as the nearest note plus pitch bend.
 // Zones may stack notes on a channel: there the nearest note only.
 const int bend=routing.mode==1?bound((int)floorf((note-n)*4096.0f+.5f),-8192,8191):0;
 for(unsigned ch=0;ch<16;ch++){if(!(chans&(1u<<ch)))continue;
  if(routing.mode==1)out_bend(ch,bend);
  for(unsigned j=0;j<owed_n;j++)if(owed[j].ch==ch&&owed[j].note==n){out_push(0x80|ch,(uint8_t)n,0);owed[j]=owed[--owed_n];break;} // retrigger
  if(owed_n>=64)continue; // never send a note-on whose note-off cannot be owed
  out_push(0x90|ch,(uint8_t)n,(uint8_t)vel);owed[owed_n++]={gate,(uint8_t)ch,(uint8_t)n};stats.midi_out_notes++;}
}
static void out_advance(unsigned z){
 for(unsigned j=0;j<owed_n;){owed[j].left-=z;if(owed[j].left<=0){out_push(0x80|owed[j].ch,owed[j].note,0);owed[j]=owed[--owed_n];}else j++;}
}
