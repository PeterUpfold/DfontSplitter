/* Copyright (C) 2001-2004 by George Williams */
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

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
#define true 1
#define false 0

int getushort(FILE *f) {
    int ch1 = getc(f);
    int ch2 = getc(f);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

long getlong(FILE *f) {
    int ch1 = getc(f);
    int ch2 = getc(f);
    int ch3 = getc(f);
    int ch4 = getc(f);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

/* There's probably only one fond in the file, but there could be more so be */
/*  prepared... */
/* I want the fond: */
/*  to get the fractional widths for the SWIDTH entry on bdf */
/*  to get the font name */
/*  to get the font association tables */
/*  to get the style flags */
/* http://developer.apple.com/techpubs/mac/Text/Text-269.html */
static FOND *BuildFondList(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list) {
    long here, start = ftell(f);
    long offset, end;
    int rname = -1;
    char name[300];
    int ch1, ch2;
    int i, j, k, l, ch, rlen, cnt;
    FOND *head=NULL, *cur;
    long widoff, kernoff, styleoff, bboff, offsetstart, glyphenc;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	printf( "\nFOND ResId=%d\n", getushort(f));
	if ( feof(f)) {
	    fprintf(stderr, "EOF found in FOND list after reading %d resources of %d.\n", i, subcnt );
    break;
	}
	rname = (short) getushort(f);
	printf( " resource flags=%x\n", getc(f));
	ch1 = getc(f); ch2 = getc(f);
	offset = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);

	cur = calloc(1,sizeof(FOND));
	if ( rname!=-1 ) {
	    fseek(f,name_list+rname,SEEK_SET);
	    ch1 = getc(f);
	    fread(name,1,ch1,f);
	    name[ch1] = '\0';
	    cur->fondname = strdup(name);
	    printf( "\nFOND %s\n", name );
	}
	else printf( "\nFOND nameless\n" );

	offset += 4;
	fseek(f,offset-4,SEEK_SET);
	printf( "Resource len=%d\n", rlen = getlong(f));
	printf( "flags = %x\n", getushort(f));
	    /* 1 => mbz */
	    /* 2 => has a glyph width table */
	    /* 1<<12 => ignore global FractEnable */
	    /* 1<<13 => more on the above */
	    /* 1<<14 => don't use fractional width table */
	    /* 1<<15 => fixed width */
	printf( "famid = %d\n", getushort(f));
	cur->first = getushort(f);
	cur->last = getushort(f);
	printf( "first=%d, last=%d\n", cur->first, cur->last );
/* on a 1 point font... */
	printf( "ascent = %g\n", getushort(f)/(double) (1<<12));
	printf( "descent = %g\n", (short) getushort(f)/(double) (1<<12));
	printf( "leading = %g\n", getushort(f)/(double) (1<<12));
	printf( "widmax = %g\n", getushort(f)/(double) (1<<12) );
	if ( (widoff = getlong(f))!=0 ) widoff += offset;
	if ( (kernoff = getlong(f))!=0 ) kernoff += offset;
	if ( (styleoff = getlong(f))!=0 ) styleoff += offset;
	printf( "width table offset = %d\n", widoff==0?0:widoff-offset);
	printf( "kern table offset = %d\n", kernoff==0?0:kernoff-offset);
	printf( "style table offset = %d\n", styleoff==0?0:styleoff-offset);
	printf( "extra width values:\n" );
	printf( " plain: %d\n", getushort(f));
	printf( " bold: %d\n", getushort(f));
	printf( " italic: %d\n", getushort(f));
	printf( " underline: %d\n", getushort(f));
	printf( " outline: %d\n", getushort(f));
	printf( " shadow: %d\n", getushort(f));
	printf( " condensed: %d\n", getushort(f));
	printf( " extended: %d\n", getushort(f));
	printf( " not used: %d\n", getushort(f));
	/* internal & undefined, for international scripts = */ getlong(f);
	printf( "version=%d\n", getushort(f));
	/* not the font version, but the format of the FOND */
	cur->assoc_cnt = getushort(f)+1;
	printf( "Association cnt=%d\n", cur->assoc_cnt );
	cur->assoc = calloc(cur->assoc_cnt,sizeof(struct assoc));
	for ( j=0; j<cur->assoc_cnt; ++j ) {
	    cur->assoc[j].size = getushort(f);
	    cur->assoc[j].style = getushort(f);
	    cur->assoc[j].id = getushort(f);
	    printf( " size=%d style=%x id=%d\n", cur->assoc[j].size, cur->assoc[j].style, cur->assoc[j].id );
	}
	end = offset+rlen;
	offsetstart = ftell(f);
	bboff = 0;
	if ( widoff!=0 || kernoff!=0 || styleoff!=0 ) {
	    /* if any of these three tables exists there will be an offset table */
	    int test;
	    printf( "Offset table cnt=%d\n", cnt=getushort(f)+1 );
	    for ( j=0; j<cnt; ++j ) {
		printf( " Offset to=%d\n", test=getlong(f));
		if ( bboff==0 ) bboff = offsetstart+test;
		/* there could be other tables here, but I don't know what */
		/*  they would mean */
	    }
	    if ( bboff!=0 ) {
		fseek(f,bboff,SEEK_SET);
		printf( "Number of font bounding boxes: %d\n", cnt = getushort(f)+1);
		for ( j=0; j<cnt; ++j ) {
		    printf( "Style = %x\n", getushort(f));
		    printf( " bb left = %g\n", (short) getushort(f)/(double) (1<<12));
		    printf( " bb bottom = %g\n", (short) getushort(f)/(double) (1<<12));
		    printf( " bb right = %g\n", (short) getushort(f)/(double) (1<<12));
		    printf( " bb top = %g\n", (short) getushort(f)/(double) (1<<12));
		}
	    }
	}
	if ( widoff!=0 ) {
	    fseek(f,widoff,SEEK_SET);
	    printf( "Style widths entries: %d\n", cnt = getushort(f)+1);
	    cur->stylewidthcnt = cnt;
	    cur->stylewidths = calloc(cnt,sizeof(struct stylewidths));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylewidths[j].style = getushort(f);
		cur->stylewidths[j].widthtab = malloc((cur->last-cur->first+3)*sizeof(short));
		printf( " Style=%x\n", cur->stylewidths[j].style);
		for ( k=cur->first; k<=cur->last+2; ++k )
		    cur->stylewidths[j].widthtab[k] = getushort(f);
	    }
	    if ( cnt>0 ) {
		printf( "Widths for style %x%s\n", cur->stylewidths[0].style,
			(cnt>1?" (I'm not printing out the others)":"") );
		for ( k=cur->first; k<=cur->last; ++k )
		    printf( "Width %d (%c): %g\n", k, k>=32&&k<127?k:k>=160?k:' ',
			cur->stylewidths[0].widthtab[k]/(double)(1<<12));
	    }
	}
	if ( kernoff!=0 ) {
	    fseek(f,kernoff,SEEK_SET);
	    printf( "Style kern entries: %d\n", cnt = getushort(f)+1);
	    cur->stylekerncnt = cnt;
	    cur->stylekerns = calloc(cnt,sizeof(struct stylekerns));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylekerns[j].style = getushort(f);
		cur->stylekerns[j].kernpairs = getushort(f);
		cur->stylekerns[j].kerns = malloc(cur->stylekerns[j].kernpairs*sizeof(struct kerns));
		printf( " Style=%x kernpairs=%d\n", cur->stylekerns[j].style,cur->stylekerns[j].kernpairs);
		for ( k=0; k<cur->stylekerns[j].kernpairs; ++k ) {
		    cur->stylekerns[j].kerns[k].ch1 = getc(f);
		    cur->stylekerns[j].kerns[k].ch2 = getc(f);
		    cur->stylekerns[j].kerns[k].offset = getushort(f);
		}
	    }
	}
	if ( styleoff!=0 ) {
	    int class;
	    fseek(f,styleoff,SEEK_SET);
	    printf( "PS Font Class Flags: %x\n", class = getushort(f)); /* How to create a bold (italic, condensed) font when we don't have one */
	    if ( class&1 ) printf( "  0x1  Font name needs coordinating\n" );
	    if ( class&2 ) printf( "  0x2  Needs MacVector reencoding\n" );
	    if ( class&4 ) printf( "  0x4  Can be outlined with PaintType==2\n" );
	    if ( class&8 ) printf( "  0x8  Do not embolded by smear & white out\n" );
	    if ( class&0x10 ) printf( "  0x10 Do not embolded by smearing\n" );
	    if ( class&0x20 ) printf( "  0x20 Embolden by increasing size\n" );
	    if ( class&0x40 ) printf( "  0x40 Do not oblique font to italicize\n" );
	    if ( class&0x80 ) printf( "  0x80 No auto-condense\n" );
	    if ( class&0x100 ) printf( "  0x100 No auto-expand\n" );
	    if ( class&0x200 ) printf( "  0x200 Needs some other encoding scheme\n" );
	    printf( " Glyph encoding offset: %d\n", glyphenc = getlong(f));	/* offset from start of table */
	    /* reserved = */ getlong(f);
	    /* 48 (byte) indeces into the name table */
	    printf( "Plain index is: %d\n", getc(f));
	    printf( "Bold index is: %d\n", getc(f));
	    printf( "Italic index is: %d\n", getc(f));
	    for ( j=3; j<48; ++j )
		getc(f);
	    printf( " String count: %d\n", cnt = getushort(f));
	    /* basename length = */ k = getc(f);
	    printf( " BaseFontName: \"" );
	    for ( j=0; j<k; ++j )
		putchar(getc(f));
	    printf("\"\n" );
	    for ( l=2; l<=cnt; ++l ) {
		k = getc(f);
		printf( "String %d, length=%d: ", l, k);
		if ( k!=0 ) {
		    ch = getc(f);
		    if ( ch<32 ) {
			printf( "%d,", ch);
			for ( j=1; j<k; ++j )
			    printf( "%d,", getc(f));
		    } else {
			putchar(ch);
			for ( j=1; j<k; ++j )
			    putchar(getc(f));
		    }
		}
		putchar('\n');
	    }
	    /* Now we've got cnt pascal strings, some of which are real */
	    /*  strings, others contain formatting info */
	    /* For any given style you look up its index, in the 48 entry list*/
	    /*  add one to it */
	    /*  then load that string, the bytes in the string tell you what */
	    /*  other strings to concatenate to the base font name to get the */
	    /*  PS fontname for this style */
	    if ( glyphenc!=0 ) {
		fseek(f,styleoff+glyphenc,SEEK_SET);
		printf( "Postscript glyph-name cnt: %d\n", cnt = getushort(f));
		for ( l=1; l<=cnt; ++l ) {
		    printf( "  Map encoding 0x%02x to '", getc(f));
		    k = getc(f);
		    for ( j=0; j<k; ++j)
			putchar(getc(f));
		    putchar('\'');
		    putchar('\n');
		}
	    }
	}
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
return( head );
}

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

static void LoadNFNT(FILE *f,struct MacFontRec *font, long offset) {
    long here = ftell(f);
    long baseow;
    long ow;
    int i;

    fseek(f,offset,SEEK_SET);
   printf( "NFNT length = %d\n", getlong(f));
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
    fseek(f,here,SEEK_SET);
}

void SearchNFNTResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds) {
    long here, start = ftell(f);
    long roff;
    int rname = -1;
    char resname[256], name[300];
    int ch1, ch2;
    int i;
    int res_id;
    FOND *mine;
    struct assoc *ass;
    struct MacFontRec font;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	printf( "NFNT Resource %x ", res_id = getushort(f));
	if ( feof(f)) {
	    fprintf(stderr, "EOF found in NFNT list after reading %d resources of %d.\n", i, subcnt );
    break;
	}
	rname = (short) getushort(f);
	printf( " resource flags=%x\n", getc(f));
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);
	LoadNFNT(f,&font, roff);
	ass = NULL;
	for ( mine = fonds; mine!=NULL; mine = mine->next ) {
	    for ( i=0; i<mine->assoc_cnt; ++i )
		if ( res_id==mine->assoc[i].id ) {
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
	    fseek(f,here,SEEK_SET);
	}
	printf( "NFNT %d", res_id );
	if ( resname[0]!='\0' )
	    printf( " %s", resname );
	if ( ass!=NULL )
	    printf( "in %s size=%d\n", mine->fondname, ass->size );
	else
	    printf( "\n" );
	printf( "  type=%x first=%x, last=%x widMax=%d kMax=%d, ndescent=%d\n   rWidth=%d rHeight=%d rWords=%d\n",
		(unsigned short) font.fontType, font.firstChar, font.lastChar,
		font.widthMax, font.kernMax, font.Descent, font.fRectWidth, font.fRectHeight,
		font.rowWords );
    }
    fseek(f,start,SEEK_SET);
}

static void SearchPostscriptResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds) {
    long here = ftell(f);
    int rname = -1, tmp;
    int ch1, ch2;
    int i;
    /* I don't pretend to understand the rational behind the format of a */
    /*  postscript font. It appears to be split up into chunks where the */
    /*  maximum chunk size is 0x800 */

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	printf( "post ResId=%d\n", getushort(f));
	if ( feof(f)) {
	    fprintf(stderr, "EOF found in POST list after reading %d resources of %d.\n", i, subcnt );
    break;
	}
	tmp = (short) getushort(f);
	if ( rname==-1 ) rname = tmp;
	printf( " resource flags=%x\n", getc(f));
	ch1 = getc(f); ch2 = getc(f);
	/*offsets[i] = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));*/ getc(f);
	/* mbz = */ getlong(f);
    }
    fseek(f,here,SEEK_SET);
}

static int getttfname(FILE *ttf,char *buffer,long offset) {
    int version, isotf=false;
    int i,num, nameoffset, stringoffset;
    int fullval, famval, fullstr, famstr, fulllen, famlen, val, tag;
    int plat, spec, lang, name, len, off, ch;
    char *pt;

    offset += 4;			/* Skip the length */
    fseek(ttf,offset,SEEK_SET);
    if ( (version=getlong(ttf))==CHR('O','T','T','O'))
	isotf = true;
    else if ( version!=0x10000 && version!=CHR('t','r','u','e'))
return(false);			/* Not going to mess with ttc collections, or whatever else this might be */

    num = getushort(ttf);
    /* srange = */ getushort(ttf);
    /* esel = */ getushort(ttf);
    /* rshift = */ getushort(ttf);
    for ( i=0; i<num; ++i ) {
	tag = getlong(ttf);
	/* checksum = */ getlong(ttf);
	nameoffset = getlong(ttf)+offset;
	/* length =*/ getlong(ttf);
	if ( tag==CHR('n','a','m','e'))
    break;
    }
    if ( i==num )
return(false);

    fseek(ttf,nameoffset,SEEK_SET);
    /* format = */ getushort(ttf);
    num = getushort(ttf);
    stringoffset = nameoffset+getushort(ttf);
    fullval = famval = 0;
    for ( i=0; i<num; ++i ) {
	plat = getushort(ttf);
	spec = getushort(ttf);
	lang = getushort(ttf);
	name = getushort(ttf);
	len = getushort(ttf);
	off = getushort(ttf);
	val = 0;
	if ( plat==0 && /* any unicode semantics will do && */ lang==0 )
	    val = 1;
	else if ( plat==3 && spec==1 && lang==0x409 )	/* MS Name */
	    val = 2;
	else if ( plat==1 && spec==0 )	/* Apple name */
	    val = 3;
	if ( name==4 && val>fullval ) {
	    fullval = val;
	    fullstr = off;
	    fulllen = len;
	    if ( val==2 || val==3 )
    break;
	} else if ( name==1 && val>famval ) {
	    famval = val;
	    famstr = off;
	    famlen = len;
	}
    }
    if ( fullval==0 ) {
	if ( famval==0 )
return( false );
	fullstr = famstr;
	fulllen = famlen;
    }

    fseek(ttf,stringoffset+fullstr,SEEK_SET);
    pt = buffer;
    if ( val==3 ) {
	for ( i=0; i<len; ++i ) {
	    ch = getc(ttf);
	    /* avoid characters that are hard to manipulate on the command line */
	    if ( ch>'!' && ch!='*' && ch!='?' && ch!='/' && ch!='\\' && ch<0x7f )
		*pt++ = ch;
	}
    } else {
	for ( i=0; i<len/2; ++i ) {
	    /* Ignore high unicode byte */ getc(ttf)/*<<8*/;
	    ch = getc(ttf);
	    /* avoid characters that are hard to manipulate on the command line */
	    if ( ch>'!' && ch!='*' && ch!='?' && ch!='/' && ch!='\\' && ch<0x7f )
		*pt++ = ch;
	}
    }
    *pt = '\0';
    /* strcpy(pt,isotf?".otf":".ttf"); */
return( true );
}

static void SearchTtfResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds) {
    long start = ftell(f), here;
    long roff;
    int rname = -1;
    int ch1, ch2;
    int i;
    char buffer[200];

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	printf( "sfnt res=%d\n", getushort(f) );
	if ( feof(f)) {
	    fprintf(stderr, "EOF found in sfnt list after reading %d resources of %d.\n", i, subcnt );
    break;
	}
	rname = (short) getushort(f);
	printf( " resource flags=%x\n", getc(f));
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);
	if ( getttfname(f,buffer,roff) )
	    printf( "\tFontName=%s\n", buffer);
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
}

static int IsResourceFork(FILE *f, long offset) {
    /* If it is a good resource fork then the first 16 bytes are repeated */
    /*  at the location specified in bytes 4-7 */
    /* We include an offset because if we are looking at a mac binary file */
    /*  the resource fork will actually start somewhere in the middle of the */
    /*  file, not at the beginning */
    unsigned char buffer[16], buffer2[16];
    long rdata_pos, map_pos, type_list, name_list, rpos;
    unsigned long tag;
    int i, cnt, subcnt;
    FOND *fondlist=NULL;

    fseek(f,offset,SEEK_SET);
    if ( fread(buffer,1,16,f)!=16 )
return( false );
    rdata_pos = offset + ((buffer[0]<<24)|(buffer[1]<<16)|(buffer[2]<<8)|buffer[3]);
    map_pos = offset + ((buffer[4]<<24)|(buffer[5]<<16)|(buffer[6]<<8)|buffer[7]);
    fseek(f,map_pos,SEEK_SET);
    buffer2[15] = buffer[15]+1;	/* make it be different */
    if ( fread(buffer2,1,16,f)!=16 )
return( false );

/* Apple's data fork resources appear to have a bunch of zeroes here instead */
/*  of a copy of the first 16 bytes */
    for ( i=0; i<16; ++i )
	if ( buffer2[i]!=0 )
    break;
    if ( i!=16 )
	for ( i=0; i<16; ++i )
	    if ( buffer[i]!=buffer2[i] )
return( false );
    getlong(f);		/* skip the handle to the next resource map */
    getushort(f);	/* skip the file resource number */
    getushort(f);	/* skip the attributes */
    type_list = map_pos + getushort(f);
    name_list = map_pos + getushort(f);

    fseek(f,type_list,SEEK_SET);
    cnt = getushort(f)+1;
    for ( i=0; i<cnt; ++i ) {
	tag = getlong(f);
	subcnt = getushort(f)+1;
	rpos = type_list+getushort(f);
	if ( tag==CHR('F','O','N','D'))
	    fondlist = BuildFondList(f,rpos,subcnt,rdata_pos,name_list);
    }

    fseek(f,type_list,SEEK_SET);
    cnt = getushort(f)+1;
    for ( i=0; i<cnt; ++i ) {
	tag = getlong(f);
 printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );
	subcnt = getushort(f)+1;
	rpos = type_list+getushort(f);
	if ( tag==CHR('P','O','S','T'))		/* No FOND */
	    SearchPostscriptResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('F','O','N','T'))	/* No FOND */
	    ;
	else if ( tag==CHR('N','F','N','T'))	/* Has FOND */
	    SearchNFNTResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('s','f','n','t'))	/* Has FOND */
	    SearchTtfResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('F','O','N','D'))
	    ;
    }
return( true );
}

static int HasResourceFork(char *filename) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
    /* linux has an HFS+ driver (or whatever) too, so we might as well always */
    /*  do this check */
    char *respath = malloc(strlen(filename)+strlen("/rsrc")+1);
    FILE *temp;
    int ret = false;

    strcpy(respath,filename);
    strcat(respath,"/rsrc");
    temp = fopen(respath,"r");
    free(respath);
    if ( temp!=NULL ) {
	ret = IsResourceFork(temp,0);
	fclose(temp);
    }
return( ret );
}

static int IsResourceInBinary(FILE *f) {
    unsigned char header[128];
    unsigned long offset;

    if ( fread(header,1,128,f)!=128 )
return( false );
    if ( header[0]!=0 || header[74]!=0 || header[82]!=0 || header[1]<=0 ||
	    header[1]>33 || header[63]!=0 || header[2+header[1]]!=0 )
return( false );
    offset = 128+((header[0x53]<<24)|(header[0x54]<<16)|(header[0x55]<<8)|header[0x56]);
return( IsResourceFork(f,offset));
}

static int lastch=0, repeat = 0;
static void outchr(FILE *binary, int ch) {
    int i;

    if ( repeat ) {
	if ( ch==0 ) {
	    /* no repeat, output a literal 0x90 (the repeat flag) */
	    lastch=0x90;
	    putc(lastch,binary);
	} else {
	    for ( i=1; i<ch; ++i )
		putc(lastch,binary);
	}
	repeat = 0;
    } else if ( ch==0x90 ) {
	repeat = 1;
    } else {
	putc(ch,binary);
	lastch = ch;
    }
}

static int IsResourceInHex(FILE *f) {
    /* convert file from 6bit to 8bit */
    /* interesting data is enclosed between two colons */
    FILE *binary = tmpfile();
    char *sixbit = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
    int ch, val, cnt, i, dlen, rlen, ret;
    char header[20], *pt;

    if ( binary==NULL ) {
	fprintf( stderr, "can't create temporary file\n" );
return( false );
    }

    lastch = repeat = 0;
    while ( (ch=getc(f))!=':' );	/* There may be comments before file start */
    cnt = val = 0;
    while ( (ch=getc(f))!=':' ) {
	if ( isspace(ch))
    continue;
	for ( pt=sixbit; *pt!=ch && *pt!='\0'; ++pt );
	if ( *pt=='\0' ) {
	    fclose(binary);
return( false );
	}
	val = (val<<6) | (pt-sixbit);
	if ( ++cnt==4 ) {
	    outchr(binary,(val>>16)&0xff);
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	    val = cnt = 0;
	}
    }
    if ( cnt!=0 ) {
	if ( cnt==1 )
	    outchr(binary,val<<2);
	else if ( cnt==2 ) {
	    val<<=4;
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	} else if ( cnt==3 ) {
	    val<<=6;
	    outchr(binary,(val>>16)&0xff);
	    outchr(binary,(val>>8)&0xff);
	    outchr(binary,val&0xff);
	}
    }

    rewind(binary);
    ch = getc(binary);	/* Name length */
    /* skip name */
    for ( i=0; i<ch; ++i )
	getc(binary);
    if ( getc(binary)!='\0' ) {
	fclose(binary);
return( false );
    }
    fread(header,1,20,binary);
    dlen = (header[10]<<24)|(header[11]<<16)|(header[12]<<8)|header[13];
    rlen = (header[14]<<24)|(header[15]<<16)|(header[16]<<8)|header[17];
    if ( rlen==0 ) {
	fclose(binary);
return( false );
    }

    ret = IsResourceFork(binary,ftell(binary)+dlen+2);
    fclose(binary);
return( ret );
}

static int IsResourceInFile(char *filename) {
    FILE *f;
    char *spt, *pt;
    int ret;

    f = fopen(filename,"r");
    if ( f==NULL )
return( false );
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename;
    pt = strrchr(spt,'.');
    if ( pt!=NULL && (pt[1]=='b' || pt[1]=='B') && (pt[2]=='i' || pt[2]=='I') &&
	    (pt[3]=='n' || pt[3]=='N') && pt[4]=='\0' ) {
	if ( IsResourceInBinary(f)) {
	    fclose(f);
return( true );
	}
    } else if ( pt!=NULL && (pt[1]=='h' || pt[1]=='H') && (pt[2]=='q' || pt[2]=='Q') &&
	    (pt[3]=='x' || pt[3]=='X') && pt[4]=='\0' ) {
	if ( IsResourceInHex(f)) {
	    fclose(f);
return( true );
	}
    }

    ret = IsResourceFork(f,0);
    fclose(f);
    if ( !ret )
	ret = HasResourceFork(filename);
return( ret );
}

static int FindResourceFile(char *filename) {
    char *spt, *pt, *dpt;
    char buffer[1400];

    if ( IsResourceInFile(filename))
return( true );

    /* Well, look in the resource fork directory (if it exists), the resource */
    /*  fork is placed there in a seperate file on non-Mac disks */
    strcpy(buffer,filename);
    spt = strrchr(buffer,'/');
    if ( spt==NULL ) { spt = buffer; pt = filename; }
    else { ++spt; pt = filename + (spt-buffer); }
    strcpy(spt,"resource.frk/");
    strcat(spt,pt);
    if ( IsResourceInFile(buffer))
return( true );

    /* however the resource fork does not appear to long names properly */
    /*  names are always lower case 8.3, do some simple things to check */
    spt = strrchr(buffer,'/')+1;
    for ( pt=spt; *pt; ++pt )
	if ( isupper( *pt ))
	    *pt = tolower( *pt );
    dpt = strchr(spt,'.');
    if ( dpt==NULL ) dpt = spt+strlen(spt);
    if ( dpt-spt>8 || strlen(dpt)>4 ) {
	char exten[8];
	strncpy(exten,dpt,7);
	exten[4] = '\0';	/* it includes the dot */
	if ( dpt-spt>6 )
	    dpt = spt+6;
	*dpt++ = '~';
	*dpt++ = '1';
	strcpy(dpt,exten);
    }
return( IsResourceInFile(buffer));
}

int main(int argc, char **argv) {
    int i, ret = 0;

    for ( i=1; i<argc; ++i )
	if ( !FindResourceFile(argv[i])) {
	    fprintf( stderr, "Can't find an appropriate resource fork in %s\n", argv[i]);
	    ret=1;
	}
return( ret );
}
