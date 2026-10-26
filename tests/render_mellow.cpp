#include "dsp/dreams.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
static float a[256];
static void u16(FILE*f,unsigned v){fputc(v&255,f);fputc(v>>8,f);}static void u32(FILE*f,unsigned v){u16(f,v);u16(f,v>>16);}
int main(int argc,char**argv){assert(argc==2);for(int id=65;id<68;id++){
 md_init();md_command(MD_SCENE,0,id);md_render(a,128);char name[1024];snprintf(name,sizeof name,"%s/%s.wav",argv[1],md_biome_name(id));FILE*f=fopen(name,"wb");assert(f);unsigned n=44100*24;fwrite("RIFF",1,4,f);u32(f,36+n*4);fwrite("WAVEfmt ",1,8,f);u32(f,16);u16(f,1);u16(f,2);u32(f,44100);u32(f,176400);u16(f,4);u16(f,16);fwrite("data",1,4,f);u32(f,n*4);md_command(MD_PLAY,0,1);double sum=0;float peak=0;
 for(unsigned at=0;at<n;at+=128){unsigned z=n-at<128?n-at:128;md_render(a,z);for(unsigned j=0;j<z*2;j++){assert(std::isfinite(a[j])&&fabsf(a[j])<1);peak=fmaxf(peak,fabsf(a[j]));sum+=a[j]*a[j];u16(f,(uint16_t)(int16_t)(a[j]*32767));}}
 fclose(f);printf("%s peak=%.5f RMS=%.5f\n",md_biome_name(id),peak,sqrt(sum/(n*2)));
}}
