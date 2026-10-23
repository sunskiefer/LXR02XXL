/*
 * clockSync.h
 *
 *  Created on: 02.02.2013
 *      Author: Julian
 *
 *      used to keep track of incoming midi clocks
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#ifndef CLOCKSYNC_H_
#define CLOCKSYNC_H_

#include "stm32f7xx_hal.h"
#include "Soundengine/globals.h"
#include "Soundengine/config.h"
#include "Soundengine/Sequencer/sequencer.h"

#define WINDOW_SIZE 8
#define WINDOW_MASK 0x07


/*
 we have a midi clock resolution of 24 ppq
 our sequencer is running with 32 ppq
 24/32 => 12/16 => 6/8 => 3/4 ratio

 for every 3 midi clocks we have to advance 4 steps
 */

//called by midi clock
void sync_tick();
void sync_midiResetPosition();
void sync_midiStartStop(uint8_t isStart);
uint8_t sync_getClockCnt();
extern uint32_t sync_delayedStepTimer;

#endif /* CLOCKSYNC_H_ */
