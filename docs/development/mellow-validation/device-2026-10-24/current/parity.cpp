#include "dsp/dreams.h"
#include <cstdio>
int main(int argc,char**argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"wb");if(!f)return 3;float a[128];
 for(int model=0;model<MD_MODELS;model++)for(int filter=0;filter<4;filter++){
  md_init();md_command(MD_SCENE,0,64);md_render(a,64);
  for(int t=0;t<6;t++){
   md_command(MD_MODEL,t,model);md_command(MD_FILTER,t,filter);
   md_command(MD_CUTOFF,t,150+t*140);md_command(MD_RESONANCE,t,t*180);
   md_command(MD_ATTACK,t,t*60);md_command(MD_DECAY,t,700);
   md_command(MD_LEVEL,t,500);md_command(MD_SEND,t,400);
   md_command(MD_MUTE,t,0);md_render(a,64);
  }
  for(int b=0;b<1000;b++){
   if(b%100==0)for(int t=0;t<6;t++)md_command(MD_TRIGGER,t,36+t*7+(b/100)%5);
   if(b%137==0)for(int t=0;t<6;t++)md_command(MD_CUTOFF,t,(b*31+t*97)%1001);
   md_render(a,64);fwrite(a,sizeof a,1,f);
  }
 }fclose(f);return 0;
}
