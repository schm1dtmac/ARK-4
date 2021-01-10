/*
 * This file is part of PRO CFW.

 * PRO CFW is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * PRO CFW is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PRO CFW. If not, see <http://www.gnu.org/licenses/ .
 */

#ifndef FLASHPATCH_H
#define FLASHPATCH_H

#include "lflash0.h"
#include "sdk.h"
#include "globals.h"
#include "functions.h"
#include "module2.h"

// Vita Flash0 patch

int patchFlash0Archive();

// 2.10+ patch
u64 kermit_flash_load(int cmd);
int flash_load();
int flashLoadPatch(int cmd);

#endif
