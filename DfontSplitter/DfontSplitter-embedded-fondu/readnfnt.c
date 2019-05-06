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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "macfonts.h"

const char *macnames[] = {
 NULL, "Eth", "eth", "Lslash", "lslash", "Scaron", "scaron", "Yacute",
 "yacute", NULL, NULL, "Thorn", "thorn", NULL, "Zcaron", "zcaron",
 NULL, NULL, NULL, NULL, NULL, "onehalf", "onequarter", "onesuperior",
 "threequarters", "threesuperior", "twosuperior", "brokenbar", "minus", "multiply", NULL, NULL,
 "space", "exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand", "quotesingle",
 "parenleft", "parenright", "asterisk", "plus", "comma", "hyphen", "period", "slash",
 "zero", "one", "two", "three", "four", "five", "six", "seven",
 "eight", "nine", "colon", "semicolon", "less", "equal", "greater", "question",
 "at", "A", "B", "C", "D", "E", "F", "G",
 "H", "I", "J", "K", "L", "M", "N", "O",
 "P", "Q", "R", "S", "T", "U", "V", "W",
 "X", "Y", "Z", "bracketleft", "backslash", "bracketright", "asciicircum", "underscore",
 "grave", "a", "b", "c", "d", "e", "f", "g",
 "h", "i", "j", "k", "l", "m", "n", "o",
 "p", "q", "r", "s", "t", "u", "v", "w",
 "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", NULL,
 "Adieresis", "Aring", "Ccedilla", "Eacute", "Ntilde", "Odieresis", "Udieresis", "aacute",
 "agrave", "acircumflex", "adieresis", "atilde", "aring", "ccedilla", "eacute", "egrave",
 "ecircumflex", "edieresis", "iacute", "igrave", "icircumflex", "idieresis", "ntilde", "oacute",
 "ograve", "ocircumflex", "odieresis", "otilde", "uacute", "ugrave", "ucircumflex", "udieresis",
 "dagger", "degree", "cent", "sterling", "section", "bullet", "paragraph", "germandbls",
 "registered", "copyright", "trademark", "acute", "dieresis", "notequal", "AE", "Oslash",
 "infinity", "plusminus", "lessequal", "greaterequal", "yen", "mu", "partialdiff", "summation",
 "product", "pi", "integral", "ordfeminine", "ordmasculine", "Omega", "ae", "oslash",
 "questiondown", "exclamdown", "logicalnot", "radical", "florin", "approxequal", "Delta", "guillemotleft",
 "guillemotright", "ellipsis", "nbspace", "Agrave", "Atilde", "Otilde", "OE", "oe",
 "endash", "emdash", "quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide", "lozenge",
 "ydieresis", "Ydieresis", "fraction", "currency", "guilsinglleft", "guilsinglright", "fi", "fl",
 "daggerdbl", "periodcentered", "quotesinglbase", "quotedblbase", "perthousand", "Acircumflex", "Ecircumflex", "Aacute",
 "Edieresis", "Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex",
 "apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave", "dotlessi", "circumflex", "tilde",
 "macron", "breve", "dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek", "caron",
NULL};

static unsigned char mac2iso[] = {
 0, 208, 240, 0, 0, 0, 0, 221, 253, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 0,
 196, 197, 199, 201, 209, 214, 220, 225, 224, 226, 228, 227, 229, 231, 233, 232,
 234, 235, 237, 236, 238, 239, 241, 243, 242, 244, 246, 245, 250, 249, 251, 252,
 0, 176, 162, 163, 167, 0, 182, 223, 174, 169, 0, 180, 168, 0, 198, 216,
 0, 177, 0, 0, 165, 181, 0, 0, 0, 0, 0, 170, 186, 0, 230, 248,
 191, 161, 172, 0, 0, 0, 0, 171, 187, 0, 32, 192, 195, 213, 0, 0,
 0, 0, 0, 0, 0, 0, 247, 0, 255, 0, 0, 164, 0, 0, 0, 0,
 0, 183, 0, 0, 0, 194, 202, 193, 203, 200, 205, 206, 207, 204, 211, 212,
 0, 210, 218, 219, 217, 0, 0, 0, 175, 0, 0, 0, 184, 0, 0, 0
};

static unsigned char iso2mac[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 32, 193, 162, 163, 219, 180, 0, 164, 172, 169, 187, 199, 194, 0, 168, 248,
 161, 177, 0, 0, 171, 181, 166, 225, 252, 0, 188, 200, 0, 0, 0, 192,
 203, 231, 229, 204, 128, 129, 174, 130, 233, 131, 230, 232, 237, 234, 235, 236,
 1, 132, 241, 238, 239, 205, 133, 0, 175, 244, 242, 243, 134, 0, 0, 167,
 136, 135, 137, 139, 138, 140, 190, 141, 143, 142, 144, 145, 147, 146, 148, 149,
 2, 150, 152, 151, 153, 155, 154, 214, 191, 157, 156, 158, 159, 0, 0, 216
};
const char *styles[]= { "Bold", "Italic", "Underline", "Outline",
    "Shadow", "Condensed", "Extended", NULL };

struct MacFontRec {
   short fontType;
   short firstChar;
   short lastChar;
   short widthMax;
   short kernMax;		/* bb learing */
   short Descent;		/* maximum negative distance below baseline*/
   short fRectWidth;		/* bounding box width */
   short fRectHeight;		/* bounding box height */
   unsigned short *offsetWidths;/* offset to start of offset/width table */
   	/* 0xffff => undefined, else high byte is offset in locTable, */
   	/*  low byte is width */
   short ascent;
   short descent;
   short leading;
   short rowWords;		/* shorts per row */
   unsigned short *fontImage;	/* rowWords*fRectHeight */
   	/* Images for all characters plus one extra for undefined */
   unsigned short *locs;	/* lastchar-firstchar+3 words */
   	/* Horizontal offset to start of n'th character. Note: applies */
   	/*  to each row. Missing characters have same loc as following */
};

typedef struct rect {
    short left,width, height,bottom;
} Rect;

static void LoadNFNT(FILE *f,struct MacFontRec *font, long offset) {
    long here = ftell(f);
    long baseow;
    long ow;
    int i;

    offset += 4;		/* skip over the length */
    fseek(f,offset,SEEK_SET);
    memset(font,'\0',sizeof(struct MacFontRec));
    font->fontType = getushort(f);
    font->firstChar = getushort(f);
    font->lastChar = getushort(f);
    font->widthMax = getushort(f);
    font->kernMax = (short) getushort(f);
    font->Descent = (short) getushort(f);
    font->fRectWidth = getushort(f);
    font->fRectHeight = getushort(f);
    baseow = ftell(f);
    ow = getushort(f);
    font->ascent = getushort(f);
    font->descent = getushort(f);
    if ( font->Descent>=0 ) {
	ow |= (font->Descent<<16);
	font->Descent = -font->descent;		/* Possibly overkill, but should be safe */
    }
    font->leading = getushort(f);
    font->rowWords = getushort(f);
    if ( font->rowWords!=0 ) {
	font->fontImage = NULL;
	if ( font->rowWords!=0 )
	    font->fontImage = calloc(font->rowWords*font->fRectHeight,sizeof(short));
	font->locs = calloc(font->lastChar-font->firstChar+3,sizeof(short));
	font->offsetWidths = calloc(font->lastChar-font->firstChar+3,sizeof(short));
	for ( i=0; i<font->rowWords*font->fRectHeight; ++i )
	    font->fontImage[i] = getushort(f);
	for ( i=0; i<font->lastChar-font->firstChar+3; ++i )
	    font->locs[i] = getushort(f);
	fseek(f,baseow+2*ow,SEEK_SET);
	for ( i=0; i<font->lastChar-font->firstChar+3; ++i )
	    font->offsetWidths[i] = getushort(f);
    }
    fseek(f,here,SEEK_SET);
}

static int FontHasChar(struct MacFontRec *font, int ch ) {
    if ( ch<font->firstChar || ch>font->lastChar || ch==0 || ch=='\t' || ch=='\r' )
return( false );

return ( font->offsetWidths[ch-font->firstChar]!=0xffff );
}

static int GetFontCount(struct MacFontRec *font) {
    int cnt=0, i;

    for ( i=font->firstChar; i<=font->lastChar; ++i ) {
	if ( FontHasChar(font,i) )
	    ++cnt;
    }
return( cnt );
}

static int GetFontAvgWidth(struct MacFontRec *font) {
    int cnt=0, i, wid=0,ch;
    /* Only average those characters in the encoding */

    for ( i=font->firstChar; i<=font->lastChar; ++i ) {
	if ( tolatin1 ) {
	    if ( i<256 && mac2iso[i]!=0 )
		ch = mac2iso[i];
	    else
    continue;
	} else
	    ch = i;
	if ( FontHasChar(font,ch) ) {
	    ++cnt;
	    wid += (font->offsetWidths[ch-font->firstChar]&0xff);
	}
    }
return( cnt>0?wid*10/cnt:0 );
}

static int AnyBitsSet(unsigned short *pt, int bits, int bite ) {
    int i;

    for ( i=bits; i<bite; ++i )
	if ( pt[i>>4]&(1<<(15-(i&15))) )
return( true );

return( false );
}

static int GetCharBBox(struct MacFontRec *font, int ch, Rect *bbox ) {
    unsigned short *widths = font->offsetWidths;
    unsigned short *locs = font->locs;
    unsigned short *rows = font->fontImage;
    int i;
    int ow;

    ch -= font->firstChar;
    ow = widths[ch];
    bbox->left = (ow>>8)+font->kernMax; bbox->width = locs[ch+1]-locs[ch];
    bbox->height = font->fRectHeight; bbox->bottom = -font->descent;
    for ( i=0; i<font->fRectHeight; ++i )
	if ( AnyBitsSet(rows+i*font->rowWords,locs[ch],locs[ch+1]) )
    break;
    bbox->height -= i;
    for ( i=0; i<bbox->height; ++i )
	if ( AnyBitsSet(rows+(font->fRectHeight-i-1)*font->rowWords,locs[ch],locs[ch+1]) )
    break;
    bbox->bottom += i; bbox->height -= i;
    if ( bbox->height<0 ) bbox->height = 0;
return( ow&0xff );	/* Width */
}

static void WriteRow(FILE *bdf,unsigned short *test, int bits, int bite ) {
    int i;
    int cnt=0x8, nibble=0;
    char buffer[80], *pt=buffer;

    for ( i=bits; i<bite; ++i ) {
	if ( test[i>>4]&(1<<(15-(i&15))) )
	    nibble |= cnt;
	if ( (cnt>>=1)==0 ) {
	    if ( nibble>=10 )
		*pt++ = 'A'+nibble-10;
	    else
		*pt++ = '0'+nibble;
	    cnt=0x8;
	    nibble=0;
	}
    }
    if ( cnt!=0x8 ) {
	if ( nibble>=10 )
	    *pt++ = 'A'+nibble-10;
	else
	    *pt++ = '0'+nibble;
    }
    if ( (pt-buffer)&1 )
	*pt++ = '0';		/* pad out to a byte */
    *pt++ = '\n';
    *pt = '\0';
    fputs(buffer,bdf);
}

static void WriteBitmap(FILE *bdf, struct MacFontRec *font, int ch, Rect *rct ) {
    unsigned short *locs = font->locs;
    unsigned short *rows = font->fontImage;
    int i;
    int bits, bite;
    int rowf, rowe;

    ch -= font->firstChar;
    bits = locs[ch]; bite = locs[ch+1];
    rowf = font->fRectHeight - (rct->height+font->descent+rct->bottom);
    rowe = font->fRectHeight-(rct->bottom+font->descent);
    for ( i=rowf; i<rowe; ++i )
	WriteRow(bdf,rows+i*font->rowWords,bits,bite);
}

static void WriteChar(FILE *bdf,struct MacFontRec *font,int enc, int index,
	int size, FOND *mine, struct assoc *ass) {
    Rect rct;
    int width;
    short *widths = NULL;

    if ( mine!=NULL && ass!=NULL ) {
	int i;
	/* I'm ignoring the glyph width table in the nfnt, and just looking at*/
	/*  the family table in the fond */
	for ( i=0; i<mine->stylewidthcnt; ++i )
	    if ( mine->stylewidths[i].style == ass->style ) {
		widths = mine->stylewidths[i].widthtab;
	break;
	    }
    }

    if ( index==0 )
	fprintf(bdf,"STARTCHAR .notdef\n");
    else if ( index<256 && macnames[index]!=NULL )
	fprintf(bdf,"STARTCHAR %s\n", macnames[index]);
    else
	fprintf(bdf,"STARTCHAR char%04x\n", index );
    width = GetCharBBox(font,index,&rct);
    fprintf(bdf,"ENCODING %d\n", enc );
    fprintf(bdf,"SWIDTH %ld 0\n",
	    widths==NULL?width*1000L/size:((widths[index]*1000L+(1<<11))>>12) );
    fprintf(bdf,"DWIDTH %d 0\n", width );
    fprintf(bdf,"BBX %d %d %d %d\n", rct.width, rct.height, rct.left, rct.bottom );
    fprintf(bdf,"BITMAP\n" );
    WriteBitmap(bdf,font,index,&rct);
    fprintf(bdf,"ENDCHAR\n" );
}

static void DumpNFNT2BDF(FILE *bdf,struct MacFontRec *font, char *resname,
	FOND *mine, struct assoc *ass) {
    int cnt, avg;
    int style = ass!=NULL ? ass->style : 0;
    int size = ass!=NULL ? ass->size : font->fRectHeight;
    int i;
    int dpi = 75;

    if ( size==17 || size==33 )
	dpi = 100;

    cnt = GetFontCount(font);
    avg = GetFontAvgWidth(font);
    fprintf(bdf,"STARTFONT 2.1\n");
    fprintf(bdf, "FONT -Fondu-%s-%s-%s-%s--%d-%d0-%d-%d-%s-%d-%s-1\n",
	    (mine!=NULL && mine->fondname!=NULL)?mine->fondname:resname,
	    (style&sf_bold)?"Bold":"Medium",
	    (style&sf_italic)?"I":"R",
	    (style&sf_condense)?"Condensed":(style&sf_extend)?"Extended":"Normal",
	    size, (size*72+dpi/2)/dpi, dpi, dpi,
	    (font->fontType&0xf000)==0xb000?"M":"P",
	    avg,
	    tolatin1?"ISO8859":"MacRoman" );

    fprintf(bdf, "SIZE %d %d %d\n", (size*72+dpi/2)/dpi, dpi, dpi );
    fprintf(bdf, "FONTBOUNDINGBOX %d %d %d %d\n", font->fRectWidth, font->fRectHeight, font->kernMax, -font->descent );
    fprintf(bdf, "COMMENT Created by Fondu from a mac NFNT/FONT resource\n" );
    fprintf(bdf, "STARTPROPERTIES 17\n" );
    fprintf(bdf, "FOUNDRY \"Fondu\"\n" );
    fprintf(bdf, "FAMILY_NAME \"%s\"\n", mine!=NULL && mine->fondname!=NULL?mine->fondname:resname );
    fprintf(bdf, "WEIGHT_NAME \"%s\"\n", (style&sf_bold)?"Bold":"Medium" );
    fprintf(bdf, "SLANT \"%s\"\n", (style&sf_italic)?"I":"R" );
    fprintf(bdf, "SETWIDTH_NAME \"%s\"\n", (style&sf_condense)?"Condensed":(style&sf_extend)?"Extended":"Normal" );
    fprintf(bdf, "ADD_STYLE_NAME \"\"\n" );
    fprintf(bdf, "PIXEL_SIZE %d\n", size );
    fprintf(bdf, "POINT_SIZE %d0\n", (size*72+dpi/2)/dpi );
    fprintf(bdf, "RESOLUTION_X %d\n", dpi );
    fprintf(bdf, "RESOLUTION_Y %d\n", dpi );
    fprintf(bdf, "SPACING \"%s\"\n", (font->fontType&0xf000)==0xb000?"M":"P");
    fprintf(bdf, "AVERAGE_WIDTH %d\n", avg );
    fprintf(bdf, "CHARSET_REGISTRY \"%s\"\n",tolatin1?"ISO8859":"MacRoman" );
    fprintf(bdf, "CHARSET_ENCODING \"1\"\n" );
    fprintf(bdf, "FONT_ASCENT %d\n", font->ascent );
    fprintf(bdf, "FONT_DESCENT %d\n", font->descent );
    fprintf(bdf, "FACE_NAME \"" );
    if ( *resname!='\0' ) {
	fprintf(bdf, "%s\"\n", resname );
    } else if ( mine!=NULL && mine->fondname!=NULL ) {
	fprintf(bdf, "%s", mine->fondname );
	if ( ass!=NULL ) {
	    for ( i=0; styles[i]!=NULL; ++i )
		if ( ass->style&(1<<i))
		    fprintf( bdf, "%s", styles[i]);
	}
	fprintf( bdf, "\"\n" );
    } else
	fprintf( bdf, "nameless\"\n" );
    fprintf(bdf, "ENDPROPERTIES\n" );
    fprintf(bdf, "CHARS %d\n", cnt );
    if ( tolatin1 ) {
	for ( i=0; i<=255; ++i ) {
	    int ch = iso2mac[i];
	    if ( FontHasChar(font,ch) )	/* character exists */
		WriteChar(bdf,font,i,ch,size,mine,ass);
	}
	for ( i=font->firstChar; i<=font->lastChar; ++i )
	    if ( (i>=256 || mac2iso[i]==0) && FontHasChar(font,i) )
		WriteChar(bdf,font,-1,i,size,mine,ass);
    } else {
	for ( i=font->firstChar; i<=font->lastChar; ++i )
	    if ( FontHasChar(font,i) )
		WriteChar(bdf,font,i,i,size,mine,ass);
    }
    fprintf(bdf, "ENDFONT\n" );
}

void SearchNFNTResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds) {
    long here, start = ftell(f);
    long roff;
    int rname = -1;
    char resname[256], name[300];
    int ch1, ch2;
    static int ucnt;
    int i,j;
    int res_id;
    FILE *bdf;
    FOND *mine;
    struct assoc *ass;
    struct MacFontRec font;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	res_id = getushort(f);
	rname = (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);
	LoadNFNT(f,&font, roff);
	if ( font.rowWords==0 ) {
	    /* rowWords==0 is a flag that the actual data for the bitmap */
	    /* lives in an associated sfnt (ttf) resource */
    continue;
	}
	ass = NULL;
/* The docs say that an sfnt will be found before an NFNT for a given id */
/*  that appears to be a lie. An sfnt will be found for size==0, an NFNT else */
	for ( mine = fonds; mine!=NULL; mine = mine->next ) {
	    int i;
	    for ( i=0; i<mine->assoc_cnt; ++i )
		if ( res_id==mine->assoc[i].id && mine->assoc[i].size!=0 ) {
		    ass = &mine->assoc[i];
	    break;
		}
	    if ( ass!=NULL )
	break;
	}
	resname[0] = '\0';
	if ( rname!=-1 ) {
	    fseek(f,name_list+rname,SEEK_SET);
	    ch1 = getc(f);
	    fread(resname,1,ch1,f);
	    resname[ch1] = '\0';
	    sprintf( name, "%s-%d.bdf", resname, ass!=NULL?ass->size:font.fRectHeight );
	} else if ( ass!=NULL ) {
	    if ( mine->fondname==NULL )
		sprintf( name, "Untitled%d-%d.bdf", ++ucnt, ass->size );
	    else {
		strcpy( name, mine->fondname );
		for ( j=0; styles[j]!=NULL; ++j )
		    if ( ass->style&(1<<j))
			strcat(name,styles[j]);
		sprintf( name+strlen(name), "-%d.bdf", ass->size );
	    }
	} else
	    sprintf(name,"Untitled%d-%d.bdf", ++ucnt, font.fRectHeight );
	fseek(f,here,SEEK_SET);
	if ( !cleanfilename(name))
	    /* Do Nothing */;
	else if ( (font.fontType&0xc)!=0 )
	    fprintf( stderr, "The bitmap font %s a depth greater than 1 and is ignored\n", name );
	else {
	    bdf = fopen( name,"w+" );
	    if ( bdf==NULL )
		fprintf( stderr, "Can't open %s for output\n", name );
	    else
		DumpNFNT2BDF(bdf,&font,resname,mine,ass);
	}
	free( font.offsetWidths );
	free( font.fontImage );
	free( font.locs );
    }
    fseek(f,start,SEEK_SET);
}
