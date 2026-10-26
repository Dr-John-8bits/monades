/* SPDX-License-Identifier: MIT OR GPL-3.0-only */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { MD_TRACKS=6, MD_MODELS=12, MD_RATE=44100, MD_QUANTUM=64, MD_BIOMES=77, MD_TONE_PRESETS=224, MD_BLANK_BIOME=64 };
enum { MD_KICK, MD_SNARE, MD_HAT, MD_FM, MD_VA, MD_RINGS, MD_HARMONIC, MD_CHOIR, MD_BASS, MD_ELEMENTS, MD_MELLOW, MD_SATIN };
enum { MD_MODEL, MD_TONE, MD_HARM, MD_DECAY, MD_LEVEL, MD_SEND, MD_HITS,
 MD_LENGTH, MD_ROTATE, MD_PROB, MD_NOTE, MD_PAN, MD_MUTE, MD_SPEED,
 MD_MACRO, MD_TEMPO, MD_PLAY, MD_LOCK, MD_MUTATE, MD_RETURN, MD_SCENE,
 MD_FREEZE, MD_GRAIN_SIZE, MD_GRAIN_POS, MD_FEEDBACK, MD_QUALITY, MD_TRIGGER, MD_PANIC,
 MD_ATTACK,MD_SWING,MD_RATCHET,MD_ROOT,MD_SCALE,MD_TRANSPOSE,MD_GRAIN_PITCH,
 MD_REVERSE,MD_DUCK_SOURCE,MD_DUCK,MD_DUCK_RELEASE,MD_ACCENT,MD_SHAPE,MD_CUTOFF,MD_RESONANCE,MD_FILTER,MD_BREATHE,MD_TONE_PRESET,MD_DELAY_SEND,MD_REVERB_SEND,MD_DELAY_TIME,MD_DELAY_FB,MD_REVERB_TIME,MD_FX_LEVEL,MD_BASS_MODE,MD_BASS_PATTERN,MD_GLIDE,MD_MARBLES,MD_TIDES,MD_TIDES_RATE,MD_WARPS,MD_BASS_STEP,MD_BASS_VALUE,MD_CLOCK_SOURCE,MD_MIDI_QUANTIZE,MD_COMMAND_COUNT };
typedef struct { int32_t model,tone,harm,decay,level,send,hits,length,rotate,prob,note,pan,mute,speed,attack,ratchet,accent,shape,cutoff,resonance,filter,breathe; } md_track;
typedef struct { int32_t delay_send,reverb_send,bass_mode,pattern,glide,marbles,tides,tides_rate,warps,edit_step; int32_t notes[16]; } md_extension;
typedef struct {
 uint32_t magic,version,seed; int32_t macro[4],tempo,locked,freeze,grain_size,grain_pos,feedback,quality;
 md_track track[MD_TRACKS];
 int32_t root,scale,transpose,swing,grain_pitch,reverse,duck_source,duck_amount,duck_release;
 int32_t biome_id,tone_preset[6]; uint32_t edited_mask;
 md_extension ext[6]; int32_t delay_ms,delay_fb,reverb_time,fx_level;
} md_patch;
typedef struct { uint32_t frames,step,events[6],dropped_commands,nonfinite; float peak; uint32_t grain_writes,voice_quanta,skipped_quanta; int32_t last_note[6]; uint32_t quality_reductions,element_modes; float track_peak[6];
 /* Monades meters, output-referred like the final mix: grain/delay/room returns, master L/R, pre-limiter peak. */
 float fx_peak[3],out_peak[2],limit_peak; } md_stats;
/* Monades granular view: tape overview (256 columns, oldest wraps at the write head), grains as samples behind the head. */
typedef struct { uint8_t peak[256]; uint32_t head,valid,length; int32_t freeze; float zone,zone_len; struct { float behind,len,env; int32_t active; } grain[4]; } md_tape;
void md_tape_view(md_tape *view);
void md_init(void);
int md_command(int command,int track,int value);
unsigned md_command_space(void);
void md_midi(unsigned status,unsigned a,unsigned b,int port); /* audio owner only */
int md_clock_source(void);
int md_midi_quantize(void);
void md_render(float *stereo,unsigned n);
const md_patch *md_state(void);
const md_stats *md_status(void);
int md_playing(void);
int md_validate(const md_patch *p);
int md_load(const md_patch *p); /* only while transport stopped, at the audio owner */
int md_decode(md_patch *out,const void *p,unsigned bytes); /* pure version migration + validation */
int md_import(const void *p,unsigned bytes); /* v1/v2/v3/v4 saved patches, stopped only */
unsigned md_memory_bytes(void);
int md_has_envelope(int model);
int md_cutoff_hz(int value);
int md_hit(unsigned step,unsigned hits,unsigned length,unsigned rotate);
const char *md_model_name(int model);
const char *md_biome_name(int id);
const char *md_biome_family(int id);
const char *md_tone_name(int id);
int md_tone_model(int id);
const char *md_scale_name(int scale);
int md_quantize(int note,int root,int scale);
#ifdef __cplusplus
}
#endif
