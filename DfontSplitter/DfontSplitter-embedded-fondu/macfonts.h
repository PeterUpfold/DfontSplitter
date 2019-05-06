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

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
#define true 1
#define false 0

#define forever for (;;)

enum style_flags { sf_bold = 1, sf_italic = 2, sf_underline = 4, sf_outline = 8,
	sf_shadow = 0x10, sf_condense = 0x20, sf_extend = 0x40 };

typedef struct fond {
    char *fondname;
    int first, last;
    int assoc_cnt;
    struct assoc {
	short size, style, id;
    } *assoc;
	/* size==0 => scalable */
	/* style>>8 is the bit depth (0=>1, 1=>2, 2=>4, 3=>8) */
	/* search order for ID is sfnt, NFNT, FONT */
    int stylewidthcnt;
    struct stylewidths {
	short style;
	short *widthtab;		/* 4.12 fixed number with the width specified as a fraction of an em */
    } *stylewidths;
    int stylekerncnt;
    struct stylekerns {
	short style;
	int kernpairs;
	struct kerns {
	    unsigned char ch1, ch2;
	    short offset;		/* 4.12 */
	} *kerns;
    } *stylekerns;
    char *psnames[48];
    char *family;
    struct fond *next;
} FOND;

typedef struct PSFONT {
    char *fontname, *familyname, *weight, *fullname, *notice, *version;
    double italicangle;
    double em;
    double fbb[4];
    int as, ds, ch, xh;
    int isadobestd;
    int glyphcnt;
    struct bbglyph {
	char *glyphname;
	int top, bottom, left, right;
	int hadvance;
	int isref;
    } *glyphs;
    short encoding[256];		/* glyph ids */
    void *temp;
} PSFONT;

extern int tolatin1;
extern const char *macnames[];
extern const char *styles[];

extern int getushort(FILE *f);
extern long getlong(FILE *f);
extern int cleanfilename(char *filename);

extern void SearchNFNTResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds);

extern void ParsePfb(FILE *pfb,PSFONT *psfont);
