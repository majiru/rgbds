/*
 * This file is part of RGBDS.
 *
 * Copyright (c) 1997-2021, RGBDS contributors.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef RGBDS_ERROR_H
#define RGBDS_ERROR_H

#include "helpers.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __plan9__
void warn(char const *fmt, ...);
void warnx(char const *fmt, ...);
void err(char const *fmt, ...);
void errx(char const *fmt, ...);
#else

void warn(char const NONNULL(fmt), ...) format_(printf, 1, 2);
void warnx(char const NONNULL(fmt), ...) format_(printf, 1, 2);

_Noreturn void err(char const NONNULL(fmt), ...) format_(printf, 1, 2);
_Noreturn void errx(char const NONNULL(fmt), ...) format_(printf, 1, 2);

#endif

#ifdef __cplusplus
}
#endif

#endif // RGBDS_ERROR_H
