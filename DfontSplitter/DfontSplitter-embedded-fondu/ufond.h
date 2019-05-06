/* Copyright (C) 2001-2003 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>		/* for NULL */
#include <stdlib.h>		/* for free */
#include <limits.h>

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
#define true 1
#define false 0

#define forever for (;;)

#if INT_MAX==2147483647
typedef int		int32;
typedef unsigned int	uint32;
#else
typedef long		int32;
typedef unsigned long	uint32;
#endif
/* I don't know of any systems where the following are not true */
typedef short		int16;
typedef unsigned short	uint16;
typedef signed char	int8;
typedef unsigned char	uint8;

enum style_flags { sf_bold = 1, sf_italic = 2, sf_underline = 4, sf_outline = 8,
	sf_shadow = 0x10, sf_condense = 0x20, sf_extend = 0x40 };
enum psstyle_flags { psf_bold = 1, psf_italic = 2, psf_outline = 4,
	psf_shadow = 0x8, psf_condense = 0x10, psf_extend = 0x20 };

typedef struct face {
    char *filename;
    enum face_type { ft_bdf, ft_ps, ft_ttf } type;	/* ttf will include otf */
    char *fontname;
    char *family;
    int16 style;
    int16 psstyle;
    int size;
    int fixed;
    short metrics[256];		/* Not computed until NFNT resource dumped, TTF knows once names are read, PS never knows */
    int id;			/* NFNT, sfnt resource ID */
    int ascent, descent, linegap;
    struct face *next;
    int xmin, ymin, xmax, ymax;	/* Bounding box for a 1em font (fixed 4.12 format) */
} Face;

typedef struct family {
    char *familyname;
    int id;			/* FOND resource ID */
    int fixed;
    int ascent, descent, linegap, maxwidth;
    Face *faces[96];
    Face *ttffaces[96];
    Face *psfaces[48];
    struct family *next;
} Family;

struct resource {
    uint32 pos;
    uint8 flags;
    uint16 id;
    char *name;
    uint32 nameloc;
    uint32 nameptloc;
};

struct resourcetype {
    uint32 tag;
    struct resource *res;
    uint32 resloc;
};

struct macbinaryheader {
    char *macfilename;
    char *binfilename;		/* if macfilename is null and this is set we will figure out macfilename by removing .bin */
    uint32 type;
    uint32 creator;
};

struct macfont {
    short fRectWidth;
    short fRectHeight;
    short ascent;
    short descent;
    short nDescent;
    short leading;
    short kernMax;
    short firstChar;
    short lastChar;
    short fontType;
    short rowWords;
    short owTLoc;
    uint16 widmax;
    Face *face;
    unsigned short *widths;
    short *lbearings;
    unsigned short *locs;
    unsigned short **rows;
    unsigned short *idealwidths;	/* For the fond */
};

extern int getushort(FILE *f);
extern long getlong(FILE *f);
extern void putshort(int val, FILE *f);
extern void putlong(long val, FILE *f);

/* Postscript */
extern int PSGetNames(Face *face);
extern struct resource *PSToResources(FILE *res,Face *face);
/* TrueType */
extern int TTFGetNames(Face *face);
extern long TTFToResource(FILE *res,Face *face);
/* bdf */
extern int BDFGetNames(Face *face);
extern long BDFToResource(FILE *res,Face *face);
