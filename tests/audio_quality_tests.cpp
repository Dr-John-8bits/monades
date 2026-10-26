/* Exercise the actual internal granular mixer with controlled tape contents.
 * This translation unit replaces dreams.cpp only in this regression executable. */
#ifndef MD_DSP_SOURCE
#define MD_DSP_SOURCE "../app/dsp/dreams.cpp"
#endif
#include MD_DSP_SOURCE
#include <cstdio>
#include <cassert>
#include <initializer_list>
// A neutral/low resonance setting must not produce an audible narrow peak.
static void check_filter_response() {
 for (float cutoff : {60.f, 1000.f, 8000.f, 18000.f}) {
  for (float resonance : {0.f, .1f, .2f, 1.f}) {
   stmlib::Svf f;f.Init();f.set_f_q<stmlib::FREQUENCY_ACCURATE>(cutoff/MD_RATE,filter_q(resonance));
   float impulse[8192];for(int i=0;i<8192;i++)impulse[i]=f.Process<stmlib::FILTER_MODE_LOW_PASS>(i==0?1.f:0.f);
   double peak=0;for(int k=0;k<=256;k++){double re=0,im=0,w=3.141592653589793*k/256;for(int i=0;i<8192;i++){re+=impulse[i]*__builtin_cos(w*i);im-=impulse[i]*__builtin_sin(w*i);}double a=__builtin_sqrt(re*re+im*im);if(a>peak)peak=a;}
   assert(isfinite(peak));if(resonance<=.2f)assert(peak<1.03);else assert(peak<3.0);
   printf("filter cutoff=%.0f resonance=%.2f peak_gain_db=%.4f\n",cutoff,resonance,20*__builtin_log10(peak));
  }
 }
}
static void check_clean_voice() {
 md_init();md_patch p=*md_state();p.scale=0;p.quality=1;p.macro[2]=500;p.macro[3]=0;
 for(int i=0;i<6;i++){md_track &t=p.track[i];t.model=MD_FM;t.mute=i!=0;t.filter=0;t.attack=0;t.decay=1000;t.level=1000;t.pan=500;t.tone=0;t.harm=234;t.shape=500;}
 assert(md_load(&p));md_command(MD_TRIGGER,0,69);float lr[256];for(int b=0;b<40;b++)md_render(lr,128);
 static float signal[44100];unsigned at=0;float peak=0;while(at<44100){unsigned n=44100-at;if(n>128)n=128;md_render(lr,n);for(unsigned k=0;k<n;k++){float x=.5f*(lr[2*k]+lr[2*k+1]);signal[at++]=x;peak=fmaxf(peak,fabsf(x));}}
 double fundamental=0,harmonics=0;for(int h=1;h<=12;h++){double re=0,im=0;for(int i=0;i<44100;i++){double w=6.283185307179586*i/44100;double x=signal[i]*(.5-.5*__builtin_cos(w));re+=x*__builtin_cos(w*h*440);im-=x*__builtin_sin(w*h*440);}double energy=re*re+im*im;if(h==1)fundamental=energy;else harmonics+=energy;}
 double thd=__builtin_sqrt(harmonics/fundamental);printf("Pure FM dry voice: harmonic distortion %.8f, peak %.6f\n",thd,peak);assert(peak<.3f&&peak>.1f);assert(thd<.005); // At least sixfold cleaner than the measured 3% clipped baseline.
}
int main(int argc,char**){
 setvbuf(stdout,nullptr,_IONBF,0);
 check_filter_response();
 check_clean_voice();
 bool enforce=argc>1;unsigned total_restarts=0;float worst=0;
 for(int quality:{0,1})for(int size=0;size<=1000;size+=10){
  md_init();for(int i=0;i<6;i++)patch.track[i].mute=1;patch.freeze=1;patch.quality=quality;patch.grain_size=size;patch.grain_pos=500;patch.macro[3]=1000;smooth[3]=1;
  for(unsigned i=0;i<GN;i++)tape[i]=3277;tape_valid=GN;
  unsigned restarts=0;float jump=0,prev[2]={};
  for(unsigned s=0;s<MD_RATE*3;s++){
   Grain old[4];memcpy(old,grains,sizeof old);float lr[2];quantum(lr,1);
   for(int j=0;j<4;j++)if(old[j].life&&old[j].phase<.999f&&grains[j].phase<old[j].phase)restarts++;
   if(s>MD_RATE)for(int c=0;c<2;c++)jump=fmaxf(jump,fabsf(lr[c]-prev[c]));
   memcpy(prev,lr,sizeof prev);assert(patch.quality==quality);assert(isfinite(lr[0])&&isfinite(lr[1]));
  }
  printf("constant tape quality=%d size=%d interrupted_grains=%u max_sample_step=%.9f\n",quality,size,restarts,jump);
  total_restarts+=restarts;worst=fmaxf(worst,jump);
 }
 printf("SUMMARY interrupted_grains=%u worst_step=%.9f\n",total_restarts,worst);
 if(enforce){assert(total_restarts==0);assert(worst<.002f);}
 if(enforce){
  // Live size/quality changes must never recycle an unfinished window.
  md_init();for(int i=0;i<6;i++)patch.track[i].mute=1;patch.freeze=1;tape_valid=GN;smooth[3]=1;patch.macro[3]=1000;
  for(unsigned i=0;i<GN;i++)tape[i]=3277;
  unsigned interrupted=0;
  for(unsigned s=0;s<MD_RATE*6;s++){
   if(s%2205==0){patch.grain_size=(s/2205)%2?1000:10;patch.quality=(s/4410)%2;patch.grain_pos=(s/2205)%2?0:1000;patch.grain_pitch=(s/2205)%2?24:-24;patch.reverse=(s/8820)%2;}
   Grain old[4];memcpy(old,grains,sizeof old);float lr[2];quantum(lr,1);
   for(int j=0;j<4;j++){
    if(old[j].life&&old[j].phase<.999f&&grains[j].phase<old[j].phase)interrupted++;
    if(grains[j].life&&grains[j].age<grains[j].life){float d=writehead-grains[j].pos;if(d<0)d+=GN;assert(d>3&&d<GN-3);}
   }
   assert(isfinite(lr[0])&&isfinite(lr[1]));
  }
  assert(interrupted==0);puts("PASS live size / quality / position / pitch / reverse changes: no interrupted grains or tape seam crossings");
  assert(md_cutoff_hz(1000)>17990&&md_cutoff_hz(0)==60);
 }
 // A limiter should be transparent at ordinary signal levels.
 double err=0,signal=0;
 for(int i=0;i<44100;i++){float x=.5f*sinf(i*6.28318530718f*997/44100),y=clip(x);err+=(y-x)*(y-x);signal+=x*x;}
 printf("limiter relative error at peak 0.5: %.9f\n",sqrtf((float)(err/signal)));
 if(enforce)assert(err==0);
 return 0;
}
