/*
 * This software is based in part on the work of the Independent JPEG Group.
 *
 * The authors make NO WARRANTY or representation, either express or implied,
 * with respect to this software, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  This software is provided "AS IS", and
 * you, its user, assume the entire risk as to its quality and accuracy.
 *
 * This software is copyright (C) 1994-1996, Thomas G. Lane.
 * All Rights Reserved except as specified below.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * software (or portions thereof) for any purpose, without fee, subject to
 * these conditions:
 * (1) If any part of the source code for this software is distributed, then
 * this README file must be included, with this copyright and no-warranty
 * notice unaltered; and any additions, deletions, or changes to the original
 * files must be clearly indicated in accompanying documentation.
 * (2) If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the work
 * of the Independent JPEG Group".
 * (3) Permission for use of this software is granted only if the user accepts
 * full responsibility for any undesirable consequences; the authors accept
 * NO LIABILITY for damages of any kind.
 *
 * These conditions apply to any software derived from or based on the IJG
 * code, not just to the unmodified library.  If you use our work, you ought
 * to acknowledge us.
 *
 * Permission is NOT granted for the use of any IJG author's name or company
 * name in advertising or publicity relating to this software or products
 * derived from it.  This software may be referred to only as "the Independent
 * JPEG Group's software".
 *
 * We specifically permit and encourage the use of this software as the basis
 * of commercial products, provided that all warranty or liability claims are
 * assumed by the product vendor.
 */

/**
* @file
* reference JPEG Group's jcmarker.c.
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef _JPEG_LIB_H_
#define _JPEG_lIB_H_

#include <stdint.h>
#include "jpegenc.h"
#include "jpeg_marker.h"

static __inline void emit_byte(JpegCtx *JpegEncCtx, int val)
{
	*JpegEncCtx->BaseAddr++ = (int8_t)val;
}

static __inline void emit_2bytes(JpegCtx *JpegEncCtx, int value)
{
	emit_byte(JpegEncCtx, (value >> 8) & 0xFF);
	emit_byte(JpegEncCtx, value & 0xFF);
}

static __inline void emit_marker(JpegCtx *JpegEncCtx, JPEG_MARKER mark)
{
	emit_byte(JpegEncCtx, 0xFF);
	emit_byte(JpegEncCtx, (int) mark);
}

void emit_dqt(JpegCtx* JpegEncCtx, int index);

void emit_sof(JpegCtx* JpegEncCtx);

void emit_sos(JpegCtx* JpegEncCtx);

void jpeg_set_quant_tbl(void *handle, int quality);

#endif //_JPEG_lIB_H_

#ifdef __cplusplus
}
#endif /* __cplusplus */

