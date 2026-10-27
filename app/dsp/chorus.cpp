/* SPDX-License-Identifier: MIT OR GPL-3.0-only
 * Monades: the Rings chorus, in its own translation unit so that rings/resources.h does not redefine
 * the Plaits LUT_* macros used by dreams.cpp. */
#include <stdint.h>
#include <stddef.h>
#include "rings/dsp/fx/chorus.h"
static rings::Chorus chorus;
extern "C" void md_chorus_reset(uint16_t *line){chorus.Init(line);}
extern "C" void md_chorus_process(float *left,float *right,unsigned n,float amount,float depth){chorus.set_amount(amount);chorus.set_depth(depth);chorus.Process(left,right,n);}
