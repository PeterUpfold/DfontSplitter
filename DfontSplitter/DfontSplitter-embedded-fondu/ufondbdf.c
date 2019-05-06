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
#include <ctype.h>
#include "ufond.h"

/* Process bdf files to turn them into NFNT resources */

struct bdffont {
    char **header;
    char ***chars;
    int charmax;
    char *fontname;
    int size;
    Face *face;
};

int BDFGetNames(Face *face) {
    /* Get the font and family names by reading the fontfile */
    FILE *bdf = fopen( face->filename, "r" );
    char buffer[512];
    char *pt;

    if ( bdf==NULL ) {
	fprintf( stderr, "Can't open %s for reading\n", face->filename );
return(false);
    }
    if ( fgets(buffer,sizeof(buffer),bdf)==NULL || strncmp(buffer,"STARTFONT ",10)!=0 ) {
	fprintf( stderr, "Hmm, %s doesn't look like a bdf font", face->filename );
	fclose(bdf);
return( false );
    }
    buffer[0]='\0';
    while ( fgets(buffer,sizeof(buffer),bdf)!=NULL ) {
	if ( strncmp(buffer,"CHARS ",6)==0 )
	    /* No more meta info */
    break;
	if ( strncmp(buffer,"FAMILY_NAME \"",13)==0 ) {
	    pt = strchr(buffer+13,'"');
	    if ( pt!=NULL ) {
		*pt = '\0';
		face->family = strdup(buffer+13);
	    }
	} else if ( strncmp(buffer,"PIXEL_SIZE ",11)==0 ) {
	    face->size = strtol(buffer+11,NULL,10);
	/* It is tempting to use the bounding box value rather than the size */
	/*  that way we don't have to do any clipping, but it gives the wrong */
	/*  result. */
	} else if ( strncmp(buffer,"FONTBOUNDINGBOX ",16)==0 ) {
	    int w,h,lb,ds;
	    if ( sscanf(buffer, "FONTBOUNDINGBOX %d %d %d %d", &w, &h, &lb, &ds)==4 ) {
		face->xmin = lb;
		face->ymin = ds;
		face->xmax = lb+w;
		face->ymax = ds+h;
	    }
	} else if ( strncmp(buffer,"FONT_ASCENT ",12)==0 ) {
	    face->ascent = strtol(buffer+12,NULL,10);
	} else if ( strncmp(buffer,"FONT_DESCENT ",13)==0 ) {
	    face->descent = strtol(buffer+13,NULL,10);
	} else if ( strncmp(buffer,"SLANT \"",7)==0 ) {
	    if ( buffer[7]=='I' || buffer[7]=='O' )
		face->style |= sf_italic;
	} else if ( strncmp(buffer,"WEIGHT_NAME \"",13)==0 ) {
	    if ( strstr(buffer+13,"Bold")!=NULL || strstr(buffer+13,"BOLD")!=NULL ||
		    strstr(buffer+13,"Gras")!=NULL || strstr(buffer+13,"Fett")!=NULL ||
		    strstr(buffer+13,"Black")!=NULL || strstr(buffer+13,"Heavy")!=NULL )
		face->style |= sf_bold;
	} else if ( strncmp(buffer,"SETWIDTH_NAME \"",15)==0 ) {
	    if ( strstr(buffer+15,"Condense")!=NULL )
		face->style |= sf_condense;
	    else if ( strstr(buffer+15,"Extend")!=NULL || strstr(buffer+15,"Expand")!=NULL )
		face->style |= sf_extend;
	} else if ( strncmp(buffer,"SPACING \"",9)==0 ) {
	    if ( buffer[9]=='M' )
		face->fixed = true;
	}
    }
    fclose(bdf);
    if ( face->size==0 ) face->size = face->ascent+face->descent;
    if ( face->family==NULL || face->size==0 ) {
	fprintf( stderr, "Hmm, %s does not contain all the needed meta data\n", face->filename );
return( false );
    }
    if ( face->ascent==0 ) {
	if ( face->descent!=0 ) face->ascent = face->size-face->descent;
	else face->ascent = (8*face->size+5)/10;
    }
    if ( face->descent == 0 ) face->descent = face->size-face->ascent;
return( true );
}

static void FreeBdfFont(struct bdffont *font) {
    char **l;
    int i;

    if ( font!=NULL ) {
	for ( i=0; i<font->charmax; ++i ) if ( font->chars[i]!=NULL ) {
	    l = font->chars[i];
	    while ( *l ) free( *l++ );
	    free(font->chars[i]);
	}
	free(font->chars);
	l = font->header;
	while ( *l ) free( *l++ );
	free( font->header );
	if ( font->fontname )
	    free( font->fontname );
	free( font );
    }
}

#define MAX_WIDTH	200

static char **SlurpChar(FILE *bdffile,char *buffer, int size, char *terminator, int *val) {
    static char **list=NULL;
    static int tot=0;
    int i=0, j, enc= -1;
    char **ret;

    for (;;) {
	if ( i>=tot ) {
	    if ( list==NULL ) list = malloc((tot=60)*sizeof(char *));
	    else list = realloc(list,(tot*=2)*sizeof(char *));
	    for ( j=i; j<tot; ++j )
		list[j] = malloc(MAX_WIDTH+1);
	}
	strcpy(list[i++],buffer);
	if ( strstr(buffer,"ENCODING")==buffer )
	    sscanf(buffer,"ENCODING %d", &enc);
	if ( strstr(buffer,terminator)==buffer )
    break;
	if ( fgets(buffer,size,bdffile)==NULL )
    break;
    }

    j=i;
    if ( j<7 ) j=7;		/* Later we just assume that we've got space in the array */
    ret = malloc((j+1)*sizeof(char *));
    for ( j=0; j<i; ++j )
	ret[j] = strdup(list[j]);
    while ( j<7 )
	ret[j++] = strdup("");
    ret[i]=NULL;
    *val = enc;
return( ret );
}

static struct bdffont *SlurpFont(Face *face) {
    FILE *bdffile = fopen(face->filename,"r");
    struct bdffont *bdffont;
    char buffer[MAX_WIDTH+1];
    char **chr;
    int i, enc, last_enc= -1;

    if ( bdffile==NULL ) {
	fprintf( stderr, "Can't open %s\n", face->filename );
return( NULL );
    }
    bdffont = calloc(1,sizeof(*bdffont));
    bdffont->face = face;

    fgets(buffer,sizeof(buffer),bdffile);
    bdffont->header = SlurpChar(bdffile,buffer, sizeof(buffer), "ENDPROPERTIES", &enc);
    fgets(buffer,sizeof(buffer),bdffile);

    fgets(buffer,sizeof(buffer),bdffile);
    while ( strcmp(buffer,"ENDFONT\n")!=0 ) {
	chr = SlurpChar(bdffile,buffer, sizeof(buffer), "ENDCHAR", &enc);
	if ( enc!=-1 ) {
	    if ( enc>=bdffont->charmax ) {
		int new = enc+256;
		if ( bdffont->charmax==0 ) bdffont->chars = malloc(new*sizeof(char **));
		else bdffont->chars = realloc(bdffont->chars,new*sizeof(char **));
		for ( i=bdffont->charmax; i<new; ++i )
		    bdffont->chars[i] = NULL;
		bdffont->charmax = new;
	    }
	    bdffont->chars[enc] = chr;
	    last_enc = enc;
	}
	if ( fgets(buffer,sizeof(buffer),bdffile)==NULL )
    break;
    }
return( bdffont );
}

static void ParseBdfHeader(struct macfont *macfont, struct bdffont *bdf) {
    int h=10,w=10,lb=0,ds=0, i;

    for ( i=0; bdf->header[i]!=NULL; ++i )
    	if ( sscanf(bdf->header[i], "FONTBOUNDINGBOX %d %d %d %d", &w, &h, &lb, &ds)==4 )
    break;
    macfont->fRectWidth = w;
    macfont->fRectHeight = macfont->face->size;
    macfont->ascent = macfont->face->ascent;
    macfont->descent = macfont->face->descent;
    macfont->nDescent = -macfont->face->descent;
    macfont->leading = 0;
    macfont->kernMax = lb;
    macfont->firstChar = 0;
    macfont->lastChar = 0xff;
    macfont->fontType = macfont->face->fixed?0xb000:0x9000;
}

static int ParseCharWidths(struct macfont *macfont,int ch,char **chlist,int loc) {
    int pwidth=0, swidth=0, gwidth=0, lb=0;
    int i;

    for ( i=0 ; chlist[i]!=NULL; ++i ) {
	if ( strncmp(chlist[i],"SWIDTH", 6)==0 )
	    sscanf(chlist[i],"SWIDTH %d", &swidth );
	else if ( strncmp(chlist[i],"DWIDTH", 6)==0 )
	    sscanf(chlist[i],"DWIDTH %d", &pwidth );
	else if ( strncmp(chlist[i],"BBX", 3)==0 )
	    sscanf(chlist[i],"BBX %d %*d %d", &gwidth, &lb );
    }

    macfont->widths[ch] = pwidth;
    macfont->lbearings[ch] = lb;
    macfont->face->metrics[ch] = (((long) swidth<<12)+500)/1000;
    macfont->locs[ch] = loc;
return( loc + gwidth );
}

static void ProcessLBearings(struct macfont *macfont) {
    int i, lb=256, wmax=0;

    for ( i=0; i<255; ++i ) {
	if ( macfont->widths[i]!=0xffff && macfont->lbearings[i]<lb )
	    lb = macfont->lbearings[i];
	if ( macfont->widths[i]!=0xffff && macfont->widths[i]>wmax )
	    wmax = macfont->widths[i];
    }
    macfont->kernMax = lb;
    macfont->widmax = wmax;

    for ( i=0; i<255; ++i ) {
	if ( macfont->widths[i]!=0xffff )
	    macfont->widths[i] |= (macfont->lbearings[i]-lb)<<8;
    }

    /* These seem to be magic, don't know why */
    macfont->widths[0] = 0;
    macfont->widths['\t'] = 6;
    macfont->widths['\r'] = 0;
}

static void ParseRow(struct macfont *macfont,int ch,int row, char *hexbits) {
    int loc = macfont->locs[ch], bits=macfont->locs[ch+1]-loc;
    unsigned short word;

    if ( row>=macfont->fRectHeight || row<0 )
return;

    while ( *hexbits!='\0' && bits>0 ) {
	if ( isdigit(*hexbits)) word = (*hexbits-'0')<<12;
	else if ( *hexbits>='a' && *hexbits<='f' ) word = (*hexbits-'a'+10)<<12;
	else if ( *hexbits>='A' && *hexbits<='F' ) word = (*hexbits-'A'+10)<<12;
	else word = 0;
	++hexbits;
	if ( isdigit(*hexbits)) word |= (*hexbits-'0')<<8;
	else if ( *hexbits>='a' && *hexbits<='f' ) word |= (*hexbits-'a'+10)<<8;
	else if ( *hexbits>='A' && *hexbits<='F' ) word |= (*hexbits-'A'+10)<<8;
	else --hexbits;
	++hexbits;
	if ( isdigit(*hexbits)) word |= (*hexbits-'0')<<4;
	else if ( *hexbits>='a' && *hexbits<='f' ) word |= (*hexbits-'a'+10)<<4;
	else if ( *hexbits>='A' && *hexbits<='F' ) word |= (*hexbits-'A'+10)<<4;
	else --hexbits;
	++hexbits;
	if ( isdigit(*hexbits)) word |= (*hexbits-'0');
	else if ( *hexbits>='a' && *hexbits<='f' ) word |= (*hexbits-'a'+10);
	else if ( *hexbits>='A' && *hexbits<='F' ) word |= (*hexbits-'A'+10);
	else --hexbits;
	++hexbits;
	if ( (loc&15)==0 )
	    macfont->rows[row][loc>>4] = word;
	else {
	    macfont->rows[row][loc>>4] |= (word>>(loc&15));
	    if ( bits-(16-(loc&15))<=0 )
    break;
	    macfont->rows[row][(loc>>4)+1] = (word<<(16-(loc&15)));
	}
	loc += 16;
	bits -= 16;
    }
}
    
static void ParseBitmap(struct macfont *macfont,int ch,char **chlist) {
    int height, descent, ascent,off,i, base;

    for ( base=0 ; chlist[base]!=NULL; ++base ) {
	if ( strncmp(chlist[base],"BBX",3)==0 )
	    sscanf(chlist[base],"BBX %*d %d %*d %d", &height, &descent );
	else if ( strncmp(chlist[base],"BITMAP",6)==0 )
    break;
    }
    if ( chlist[base]!=NULL ) {
	ascent = height+descent;
	off = macfont->ascent-ascent;
	for ( i=0; i<height; ++i )
	    ParseRow(macfont,ch,i+off,chlist[i+base+1]);
    }
}

static void DummyUpFakeBitmap(struct macfont *macfont,int ch) {
    /* Create a vertical line for the implicit char displayed for missing characters */
    int loc = macfont->locs[ch], lw = (loc>>4), lb= (1<<(15-(loc&15)));
    int i;

    for ( i=0; i<macfont->fRectHeight; ++i )
	macfont->rows[i][lw] |= lb;
}

static struct macfont *CvtBdfToNfnt(struct bdffont *bdf,Face *face) {
    struct macfont *macfont;
    int i, loc, ch;

    if ( bdf==NULL )
return( NULL );
    macfont = calloc(sizeof(struct macfont),sizeof(char));
    macfont->face = face;
    macfont->widths = calloc(258,sizeof(short));
    macfont->lbearings = calloc(258,sizeof(short));
    macfont->locs = calloc(258,sizeof(short));
    ParseBdfHeader(macfont,bdf);
    macfont->rows = malloc(macfont->fRectHeight*sizeof(char *));

    loc = 0;
    for ( i=0; i<256; ++i ) {
	ch = i;
	if ( ch<bdf->charmax && bdf->chars[ch]!=NULL )
	    loc = ParseCharWidths(macfont,i,bdf->chars[ch],loc);
	else {
	    macfont->widths[i] = -1;
	    macfont->locs[i] = loc;
	}
    }
    macfont->locs[i] = loc;		/* Char for unused letters */
    macfont->widths[i]=3;
    macfont->face->metrics[i]=(3<<12)/macfont->fRectHeight;
    macfont->lbearings[i]=1;
    macfont->locs[i+1] = ++loc;	/* one beyond, gives size to last character */
    macfont->widths[i+1]=-1;

    macfont->rowWords = (loc+15)/16;
    for ( i=0; i<macfont->fRectHeight; ++i )
	macfont->rows[i] = calloc(macfont->rowWords,sizeof(short));

    for ( i=0; i<256; ++i ) {
	ch = i;
	if ( ch<bdf->charmax && bdf->chars[ch]!=NULL )
	    ParseBitmap(macfont,i,bdf->chars[ch]);
    }
    DummyUpFakeBitmap(macfont,i);

    ProcessLBearings(macfont);
return( macfont );
}

static struct macfont *ProcessBdfFile(Face *face) {
    struct bdffont *bdf = SlurpFont(face);
    struct macfont *macfont = CvtBdfToNfnt(bdf,face);
    FreeBdfFont(bdf);
return( macfont );
}

static void FreeMacFont(struct macfont *macfont) {
    int i;

    for ( i=0; i<macfont->fRectHeight; ++i )
	free(macfont->rows[i]);
    free(macfont->rows);
    free(macfont->lbearings);
    free(macfont->widths);
    free(macfont->locs);
    free(macfont);
}

static long DumpMacFont(FILE *resfile, struct macfont *macfont) {
    int size, i,j;
    long here;

    size = 13*sizeof(short) +
	    macfont->fRectHeight*macfont->rowWords*sizeof(short) +
	    258*sizeof(short) +
	    258*sizeof(short);
    macfont->owTLoc = (size- 258*sizeof(short)- (8*sizeof(short)))/
						/* Offset to owTLoc itself*/
	    sizeof(short);

    here = ftell(resfile);
    putlong(0,resfile);			/* Resources start with a size field */
    putshort(macfont->fontType,resfile);
    putshort(macfont->firstChar,resfile);
    putshort(macfont->lastChar,resfile);
    putshort(macfont->widmax,resfile);
    putshort(macfont->kernMax,resfile);
    putshort(macfont->nDescent,resfile);
    putshort(macfont->fRectWidth,resfile);
    putshort(macfont->fRectHeight,resfile);
    putshort(macfont->owTLoc,resfile);
    putshort(macfont->ascent,resfile);
    putshort(macfont->descent,resfile);
    putshort(macfont->leading,resfile);
    putshort(macfont->rowWords,resfile);
    for ( i=0; i<macfont->fRectHeight; ++i ) {
	for ( j=0; j<macfont->rowWords; ++j )
	    putshort( macfont->rows[i][j],resfile );
    }
    for ( i=0; i<258; ++i )
	putshort(macfont->locs[i],resfile);
    for ( i=0; i<258; ++i )
	putshort(macfont->widths[i],resfile);

    if ( size!=ftell(resfile)-here-4 )
	printf( "IE: expected %d found %d\n", size, ftell(resfile)-here-4 );
    size = ftell(resfile)-here-4;
    fseek(resfile,here,SEEK_SET);
    putlong(size,resfile);
    fseek(resfile,0,SEEK_END);
return( here );
}

long BDFToResource(FILE *res,Face *face) {
    struct macfont *macfont = ProcessBdfFile(face);
    long ret;
    if ( macfont==NULL )
return( 0 );
    ret = DumpMacFont(res, macfont);
    FreeMacFont(macfont);
return( ret );
}
