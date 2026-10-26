/* SPDX-License-Identifier: MIT OR GPL-3.0-only */
#define main platform_main
#include "../base/host/plat_host.c"
#undef main
#include <assert.h>
static unsigned packed(const uint8_t *src,unsigned n,uint8_t *dst){unsigned at=0;for(unsigned i=0;i<n;){unsigned k=n-i>7?7:n-i;uint8_t mask=0;for(unsigned j=0;j<k;j++)mask|=(src[i+j]>>7)<<j;dst[at++]=mask;for(unsigned j=0;j<k;j++)dst[at++]=src[i+j]&127;i+=k;}return at;}
static uint8_t response[224];static uint32_t response_n;
static int req(int cmd,const uint8_t *data,unsigned n){uint8_t p[300]={0x7d,0x4d,0x44,cmd,1};assert(n<=sizeof p-5);if(n)memcpy(p+5,data,n);response_n=0;int rc=editor_reply(p,n+5,response,&response_n);assert(response_n<=sizeof response);return rc;}
static unsigned begin(const md_patch *p,uint32_t crc){uint32_t h[]={sizeof *p,crc};uint8_t b[16];unsigned n=packed((void*)h,sizeof h,b);assert(!req(11,b,n));return response[0];}
static void chunks(const md_patch *p,unsigned token){for(unsigned off=0;off<sizeof *p;off+=196){uint8_t b[240]={token,off&127,off>>7};unsigned n=sizeof *p-off;if(n>196)n=196;n=packed((uint8_t*)p+off,n,b+3);assert(!req(12,b,n+3));}}
int main(void){
 boot();run_ms(100);dream_dismiss_intro();md_patch initial=*md_state(),desired=initial;desired.tempo=92;
 assert(!req(7,0,0)&&response_n==32);assert(!req(9,0,0)&&response_n==208);
 assert(!req(8,0,0));assert(md_stored_size==1496);assert(!memcmp(md_stored_snapshot,&initial,1496));
 dream_cmd(MD_TEMPO,0,110);run_ms(20);assert(!memcmp(md_stored_snapshot,&initial,1496));
 assert(app_backup_validate(0,&initial,sizeof initial));
 md_patch legacy=initial;legacy.version=4;assert(app_backup_validate(0,&legacy,652));
 legacy.version=5;assert(app_backup_validate(0,&legacy,1292));legacy.version=6;assert(app_backup_validate(0,&legacy,1292));assert(!app_backup_validate(0,&legacy,sizeof legacy));
 assert(md_state()->tempo==110); /* validation never loads the old project */assert(!app_backup_validate(1,&initial,sizeof initial));assert(!app_backup_validate(0,&initial,20));
 md_patch bad=initial;bad.version=77;assert(!app_backup_validate(0,&bad,sizeof bad));bad=initial;bad.track[2].model=99;assert(!app_backup_validate(0,&bad,sizeof bad));
 for(unsigned obj=0;obj<23;obj++)if(obj!=1)assert(app_backup_validate(obj,0,0));
 unsigned token=begin(&desired,dream_crc(&desired,sizeof desired));uint8_t t=token;
 assert(req(13,&t,1)==6);chunks(&desired,token);assert(!req(13,&t,1));assert(md_state()->tempo==92);
 token=begin(&initial,123);t=token;chunks(&initial,token);assert(req(13,&t,1)==6);assert(md_state()->tempo==92);
 token=begin(&initial,dream_crc(&initial,sizeof initial));t=token;now_ms+=5001;assert(req(13,&t,1)==6);
 token=begin(&initial,dream_crc(&initial,sizeof initial));t=token;chunks(&initial,token);dream_cmd(MD_PLAY,0,1);run_ms(20);assert(req(13,&t,1)==4);dream_cmd(MD_PLAY,0,0);run_ms(20);
 uint32_t writes=store_writes;app_backup_hold(1);bank_slot=15;bank_kind=0;assert(bank_action(1)==4);md_tap(B_SAVE);assert(store_writes==writes);app_backup_flush();assert(store_writes==writes);app_backup_hold(0);
 for(int kind=0;kind<2;kind++)for(int slot=0;slot<(kind?64:16);slot++){bank_kind=kind;bank_slot=slot;assert(!bank_action(2));int obj=bank_object();assert(app_backup_validate(obj,store[obj],store_len[obj]));}
 bank_kind=0;bank_slot=15;writes=store_writes;bank_armed=-1;assert(bank_action(1)==5&&store_writes==writes);assert(!bank_action(1));
 {UserBiome old=user_biome_io;old.patch.version=6;assert(app_backup_validate(18,&old,USER_BIOME_1_1));
  int obj=OBJ_USER+3;memcpy(store[obj],&old,USER_BIOME_1_1);store_len[obj]=USER_BIOME_1_1;bank_kind=0;bank_slot=3;assert(!bank_action(0));assert(md_state()->version==7&&md_state()->lfo[0][0].depth==0);} /* a slot saved by 1.1 still loads */
 UserBiome u=user_biome_io;u.name[23]='x';assert(!app_backup_validate(18,&u,sizeof u));
 ToneBank tones=tone_bank_io;tones.used=0x10000;assert(!app_backup_validate(22,&tones,sizeof tones));
 tones=tone_bank_io;tones.tone[15].ext.notes[2]=99;assert(!app_backup_validate(22,&tones,sizeof tones));
 assert(!plat_store_save(OBJ_PATCH,&desired,sizeof desired));app_backup_restored();assert(md_state()->tempo==92&&!md_playing()&&!md_clock_source()&&!dream_reloading);
 dream_cmd(MD_CLOCK_SOURCE,0,1);run_ms(20);assert(app_backup_playing());assert(!dream_install(&initial,sizeof initial));
 puts("PASS editor capabilities/state, atomic snapshot, bounded upload, CRC/version/range/timeout/busy rejection, all 16/64 slots, overwrite guard, backup save lock, saved-state reload");return 0;
}
