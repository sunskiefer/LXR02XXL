/*
 * EuklidGenerator.h
 *
 *  Created on: 04.08.2012
 *
 * Copyright (C) 2012-2026 Erica Synths
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * This file is part of the LXR firmware. It is licensed under the GNU General
 * Public License version 3, with the additional permission and terms in
 * NOTICE.md. See LICENSE and NOTICE.md in the root of the repository.
 */


#ifndef EUKLIDGENERATOR_H_
#define EUKLIDGENERATOR_H_

#include "stm32f7xx_hal.h"

void euklid_init();
uint8_t euklid_getLength(uint8_t trackNr);
uint8_t euklid_getSteps(uint8_t trackNr);
uint8_t euklid_getRotation(uint8_t trackNr);
void euklid_setLength(uint8_t trackNr, uint8_t value);
void euklid_setSteps(uint8_t trackNr, uint8_t value, uint8_t patternNr);
void euklid_setRotation(uint8_t trackNr, uint8_t value, uint8_t patternNr);
void euklid_rotatePattern(uint8_t length, uint8_t amount);
void euklid_transferPattern(uint8_t trackNr, uint8_t patternNr);
#endif /* EUKLIDGENERATOR_H_ */
