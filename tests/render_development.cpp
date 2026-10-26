#include "dsp/dreams.h"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <cassert>
static float a[256];
static void u16(FILE*f,unsigned n){fputc(n&255,f);fputc(n>>8&255,f);}static void u32(FILE*f,unsigned n){u16(f,n);u16(f,n>>16);}
int main(int argc,char**argv){assert(argc==2);const char *names[]={"Rooted Dub","Sub Tides","Moss Bells","Quiet Metal","Velvet Motion"};
 for(int demo=0;demo<5;demo++){md_init();md_command(MD_SCENE,0,64);md_render(a,128);md_patch p=*md_state();p.biome_id=-1;p.quality=1;p.tempo=demo==0?116:demo==1?90:demo==2?80:demo==3?100:108;p.root=2;p.scale=4;p.locked=1;p.macro[0]=500;p.macro[1]=100;p.macro[2]=420;p.macro[3]=100;p.fx_level=450;p.reverb_time=550;p.delay_fb=350;p.delay_ms=300;
  for(int i=0;i<6;i++){auto&t=p.track[i];t.mute=1;t.level=480;t.pan=500;t.prob=1000;t.length=16;t.hits=16;t.speed=1;t.note=38;t.attack=0;t.decay=180;t.tone=350;t.harm=220;t.shape=150;t.cutoff=550;t.filter=1;t.resonance=0;t.accent=250;t.ratchet=1;p.ext[i].reverb_send=200;}
  auto&t=p.track[0];t.mute=0;t.model=MD_BASS;t.level=700;p.ext[0].bass_mode=1;p.ext[0].glide=120;p.ext[0].marbles=200;p.ext[0].delay_send=100;
  if(demo==1){p.ext[0].tides=500;p.ext[0].tides_rate=600;t.filter=1;t.cutoff=320;p.ext[0].reverb_send=80;}
  if(demo!=1){auto&k=p.track[1];k.mute=0;k.model=MD_KICK;k.note=36;k.hits=4;k.level=600;k.decay=300;p.ext[1].reverb_send=0;}
  auto&b=p.track[2];b.mute=0;b.model=(demo==2||demo==3)?MD_ELEMENTS:MD_RINGS;b.note=62;b.level=380;b.hits=3;b.decay=400;b.filter=1;b.cutoff=650;b.send=120;p.ext[2].reverb_send=600;p.ext[2].delay_send=400;p.ext[2].tides=150;b.shape=demo==3?500:100;
  if(demo==4){auto&v=p.track[3];v.mute=0;v.model=MD_VA;v.note=50;v.level=260;v.hits=2;v.attack=280;v.decay=500;v.cutoff=420;p.ext[3].tides=350;p.ext[3].warps=150;p.ext[3].reverb_send=450;}
  assert(md_load(&p));char path[1024];snprintf(path,sizeof path,"%s/%d.patch",argv[1],demo);FILE*f=fopen(path,"wb");assert(f);fwrite(&p,1,sizeof p,f);fclose(f);
  snprintf(path,sizeof path,"%s/%d.wav",argv[1],demo);f=fopen(path,"wb");assert(f);unsigned frames=44100*16;fwrite("RIFF",1,4,f);u32(f,36+frames*4);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,2);u32(f,44100);u32(f,176400);u16(f,4);u16(f,16);fwrite("data",1,4,f);u32(f,frames*4);
  md_command(MD_PLAY,0,1);double sum=0;float peak=0;for(unsigned at=0;at<frames;at+=128){unsigned n=frames-at<128?frames-at:128;md_render(a,n);for(unsigned j=0;j<n*2;j++){assert(std::isfinite(a[j]));peak=fmaxf(peak,fabsf(a[j]));sum+=a[j]*a[j];u16(f,(uint16_t)(int16_t)(a[j]*32767));}}fclose(f);printf("%s: peak %.5f RMS %.5f quality %d\n",names[demo],peak,sqrt(sum/(frames*2)),md_state()->quality);
 }
}
