#include "plaits/dsp/drums/analog_snare_drum.h"
#include "snare-reference.h"
#include <cstring>
#include <cstdio>
int main(){
 plaits::AnalogSnareDrum fast;plaits::ReferenceSnareDrum reference;
 fast.Init();reference.Init();float a[128],b[128];unsigned samples=0;
 const unsigned lengths[]={1,2,17,32,63,64,65,128};
 for(unsigned i=0;i<10000;i++){
  unsigned n=lengths[i%8];float f=.001f+(i%191)*.0011f,tone=(i%1001)*.001f,decay=((i*13)%1001)*.001f,snap=((i*71)%1001)*.001f;
  stmlib::Random::Seed(i+7);fast.Render(false,i%17==0,.65f,f,tone,decay,snap,a,n);
  stmlib::Random::Seed(i+7);reference.Render(false,i%17==0,.65f,f,tone,decay,snap,b,n);
  if(memcmp(a,b,n*sizeof(float))){fprintf(stderr,"Mismatch block %u length %u\n",i,n);return 1;}
  samples+=n;
 }
 printf("PASS %u snare samples byte-identical over 10000 changing-parameter blocks and 8 block sizes, including reference fallback.\n",samples);
}
