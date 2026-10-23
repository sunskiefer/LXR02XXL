/*
 * copyClearTools.h
 *
 * Created: 26.01.2013 08:44:09
 *  Author: Julian
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */ 


#ifndef COPYCLEARTOOLS_H_
#define COPYCLEARTOOLS_H_

#include "main.h"

//copy/clear mode helpers
enum
{
	MODE_NONE,
	MODE_CLEAR,
	MODE_COPY_PATTERN,
	MODE_COPY_TRACK,
};

//clear targets
enum
{
	CLEAR_KIT,
	CLEAR_TRACK,
	CLEAR_PATTERN,
	CLEAR_AUTOMATION1,
	CLEAR_AUTOMATION2,
};
extern uint8_t copyClear_Mode;

void copyClear_executeClear();
void copyClear_clearCurrentTrack();
void copyClear_clearCurrentPattern();
void copyClear_copyTrack();
void copyClear_copyPattern();
uint8_t copyClear_getClearTarget();
uint8_t copyClear_isClearModeActive();
void copyClear_setClearTarget(uint8_t mode);
void copyClear_armClearMenu(uint8_t isShown);

uint8_t copyClear_srcSet();
void copyClear_setSrc(int8_t src, uint8_t type);
void copyClear_setDst(int8_t dst, uint8_t type);

void copyClear_reset();


extern uint8_t copyClear_clearTarget;
extern uint8_t copyClear_showClearMenu;

#endif /* COPYCLEARTOOLS_H_ */
