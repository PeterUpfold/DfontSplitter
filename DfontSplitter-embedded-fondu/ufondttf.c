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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ufond.h"

/* When there are bitmaps in a ttf file, we should create dummy NFNT entries */
/*  for them, each 26 bytes long which is all the fields up to rowWords. */
/*  rowWords should be 0, and there should be nothing after it. firstChar is 0*/
/*  and lastChar is 0xff. OffsetWidths is 0. others appear to be as expected */

struct ttfinfo {
    long cmap_start;
    long head_start;
    long hhea_start;
    long hmtx_start;
    long maxp_start;
    long name_start;
    long post_start;
    int max_glyph;
    int glyphs[256];		/* Glyph ids of the first 256 encoding entries */
    int emsize;
    int macstyle;
    int longmtx;
    int isfixed;
    short metrics[256];
    char *fontname, *familyname;
};

static char *ReadUnicode(FILE *ttf,long pos, int len) {
    /* len is in bytes, not unicode-chars */
    char *str = malloc(len/2+1), *pt = str;
    int ch;
    long here = ftell(ttf);
    fseek(ttf,pos,SEEK_SET);
    while ( len>0 ) {
	ch = getushort(ttf);
	if ( ch>=' ' && ch<127 )
	    *pt++ = ch;
	len -= 2;
    }
    *pt = '\0';
    fseek(ttf,here,SEEK_SET);
return( str );
}

static char *Read1Byte(FILE *ttf,long pos, int len) {
    char *str = malloc(len+1), *pt = str;
    int ch;
    long here = ftell(ttf);
    fseek(ttf,pos,SEEK_SET);
    while ( len>0 ) {
	ch = getc(ttf);
	if ( ch>=' ' && ch<127 )
	    *pt++ = ch;
	--len;
    }
    *pt = '\0';
    fseek(ttf,here,SEEK_SET);
return( str );
}

int TTFGetNames(Face *face) {
    /* Get a bunch of info including the font and family names */
    FILE *ttf = fopen( face->filename, "r" );
    int ch1, ch2, ch3, ch4;
    struct ttfinfo info;
    int i, cnt, tag;
    int platform, specific, offset, format, len, lang, name, stroff;
    long strbase;
    char *str;

    if ( ttf==NULL ) {
	fprintf( stderr, "Can't open %s for reading\n", face->filename );
return(false);
    }
    ch1 = getc(ttf);
    ch2 = getc(ttf);
    ch3 = getc(ttf);
    ch4 = getc(ttf);
    if (!(( ch1==0 && ch2==1 && ch3==0 && ch4==0 ) ||
	    (ch1=='O' && ch2=='T' && ch3=='T' && ch4=='O') ||
	    (ch1=='t' && ch2=='r' && ch3=='u' && ch4=='e')) ) {
	fprintf( stderr, "Hmm, %s doesn't look like a true or open type font.\n", face->filename );
	fclose(ttf);
return( false );
    }
    memset(&info,0,sizeof(info));

    cnt = getushort(ttf);
    /* searchRange = */ getushort(ttf);
    /* entrySelector = */ getushort(ttf);
    /* rangeshift = */ getushort(ttf);

    for ( i=0; i<cnt; ++i ) {
	tag = getlong(ttf);
	/*checksum = */getlong(ttf);
	offset = getlong(ttf);
	/*length = */getlong(ttf);
#ifdef DEBUG
 printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );
#endif
	switch ( tag ) {
	  case CHR('c','m','a','p'):
	    info.cmap_start = offset;
	  break;
	  case CHR('b','h','e','d'):
	  case CHR('h','e','a','d'):
	    info.head_start = offset;
	  break;
	  case CHR('h','h','e','a'):
	    info.hhea_start = offset;
	  break;
	  case CHR('h','m','t','x'):
	    info.hmtx_start = offset;
	  break;
	  case CHR('m','a','x','p'):
	    info.maxp_start = offset;
	  break;
	  case CHR('n','a','m','e'):
	    info.name_start = offset;
	  break;
	  case CHR('p','o','s','t'):
	    info.post_start = offset;
	  break;
	}
    }
    if ( info.cmap_start==0 || info.head_start==0 || info.hhea_start==0 ||
	    info.hmtx_start==0 || info.maxp_start==0 || info.name_start==0 ) {
	fprintf( stderr, "Hmm, %s is missing some required tables. I can't deal with it.\n", face->filename );
	fclose(ttf);
return( false );
    }

    if ( info.post_start ) {
	fseek(ttf,info.post_start+12,SEEK_SET);
	info.isfixed = getlong(ttf);
    }
    fseek(ttf,info.maxp_start+4,SEEK_SET);
    info.max_glyph = getushort(ttf);
    fseek(ttf,info.head_start+18,SEEK_SET);
    info.emsize = getushort(ttf);
    fseek(ttf,info.head_start+36,SEEK_SET);
    face->xmin = getushort(ttf);
    face->ymin = getushort(ttf);
    face->xmax = getushort(ttf);
    face->ymax = getushort(ttf);
    info.macstyle = getushort(ttf);
    fseek(ttf,info.hhea_start+4,SEEK_SET);
    face->ascent = getushort(ttf);
    face->descent = -getushort(ttf);
    face->linegap = getushort(ttf);
    fseek(ttf,info.hhea_start+34,SEEK_SET);
    info.longmtx = getushort(ttf);

    if ( face->ascent+face->descent!=info.emsize ) {
	face->ascent = .8*info.emsize;
	face->descent = info.emsize-face->ascent;
    }

	/* We need to get an encoding vector so we'll know what glyphs go to */
	/*  what encodings. We need this so we can read in the metrics info */
	/*  we need that so we can fill in the width table of the FOND */
    fseek(ttf,info.cmap_start+2,SEEK_SET);
    cnt = getushort(ttf);
    for ( i=0; i<cnt; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	offset = getlong(ttf);
	if ( platform==1 && ( specific!=1 && specific!=2 && specific!=3 && specific!=25 ))
    break;		/* Search for a single byte encoding */
    }
    fseek(ttf,info.cmap_start+offset,SEEK_SET);
    format = getushort(ttf);
    len = getushort(ttf);
    if ( i==cnt || format!=0 ) {
	fprintf( stderr, "%s has a more complicated encoding vector than I am prepared to deal with\n", face->filename );
	fclose(ttf);
return( false );
    }
    /*language = */ getushort(ttf);
    for ( i=0; i<256; ++i )
	info.glyphs[i] = getc(ttf);

    for ( i=0; i<256; ++i ) {
	int k = info.glyphs[i];
	if ( k>=info.longmtx )
	    k = info.longmtx-1;
	fseek(ttf,info.hmtx_start+k*4,SEEK_SET);
	info.metrics[i] = getushort(ttf);
    }

    fseek(ttf,info.name_start+2,SEEK_SET);
    cnt = getushort(ttf);
    strbase = getushort(ttf)+info.name_start;
    for ( i=0; i<cnt; ++i ) {
	platform = getushort(ttf);
	specific = getushort(ttf);
	lang = getushort(ttf);
	name = getushort(ttf);
	len = getushort(ttf);
	stroff = getushort(ttf);
	if ( name==1 /* Family */ || name==6 /* Postscript name */ ) {
	    if (( platform==3 && specific==1 && (lang&0xff)==9 ) ||
		    ( platform==0 /* any specific, any lang */ ) )
		str = ReadUnicode(ttf, strbase+stroff,len);
	    else if ( platform==1 && specific==0 && lang==0 )
		str = Read1Byte(ttf, strbase+stroff,len);
	    if ( name==1 )
		info.familyname = str;
	    else
		info.fontname = str;
	}
    }

    fclose(ttf);

    if ( info.fontname==NULL && info.familyname!=NULL )
	info.fontname = strdup(info.familyname);
    else if ( info.fontname==NULL ) {
	fprintf( stderr, "%s has no font name.\n", face->filename );
return( false );
    }

    face->fontname = info.fontname;
    face->family = info.familyname;
    face->style = info.macstyle;
    face->size = 0;
    face->fixed = info.isfixed;
    for ( i=0; i<256; ++i )
	face->metrics[i] = (info.metrics[i]<<12)/info.emsize;
return( true );
}
	    
long TTFToResource(FILE *res,Face *face) {
    /* Just put the ttf(otf) file into a resource */
    struct stat statb;
    int ch;
    FILE *ttf;
    long here = ftell(res);

    stat(face->filename,&statb);
    putlong(statb.st_size,res);

    ttf = fopen(face->filename,"r");
    while ( (ch=getc(ttf))!=EOF )
	putc(ch,res);
    fclose(ttf);
return( here );
}
