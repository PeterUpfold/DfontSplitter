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
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include "ufond.h"

/* ufond [-bin] [-res] [-dfont] [-macbin] [-script num] filelist */

/* The basic idea is the following: */
/*  We collect all the files we are supposed to deal with */
/*  we parse each file far enough to get: */
/*	a fontname */
/*	(for bdf) a size */
/*	(for ttf) the mac encoding so we can order the hmtx properly */
/*	(for bdf,ttf) the metrics */
/*  We strip off things like Italic, Bold, Oblique, Condensed, "-" to get a family name */
/*  We collect all files into families */
/*  We generate one FOND for each family and put the bdf and ttf into it */
/*  We generate one seperate resource file for each ps font in the family */
/*  If we do not have a plain style then make a seperate FOND for each style */
/*   and pretend each is plain */
/*  If we have a PS font style with no matching bdf (ie. no metrics) then */
/*   complain (but make an entry for it in the fond anyway?) */

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

void putshort(int val, FILE *f) {
    putc(val>>8,f);
    putc(val&0xff,f);
}

void putlong(long val, FILE *f) {
    putc((val>>24)&0xff,f);
    putc((val>>16)&0xff,f);
    putc((val>>8)&0xff,f);
    putc(val&0xff,f);
}

enum output_format { of_dfont, of_macbin, of_res } output_format = of_macbin;
static int script = 0;		/* Roman */

static uint16 HashToId(char *fontname) {
    int low = 128, high = 0x4000;
    uint32 hash = 0;

    if ( script!=0 ) {
	low = 0x4000+(script-1)*0x200;
	high = low + 0x200;
    }
    while ( *fontname ) {
	int temp = (hash>>28)&0xf;
	hash = (hash<<4) | temp;
	hash ^= *fontname++-' ';
    }
    hash %= (high-low);
    hash += low;
return( hash );
}

static void Usage(char *prog) {
    fprintf( stderr, "Usage: %s [-dfont] [-macbin] [-res] [-script name] fontfile {fontfiles}\n", prog );
    fprintf( stderr, " -dfont\t\tPuts the output into a Mac OS/X dfont file.\n" );
    fprintf( stderr, " -macbin\tPuts the output into a resource fork inside a mac binary file.\n" );
    fprintf( stderr, " -res\tPuts the output into a data file containing a resource fork\n\t\t(you have to figure out how to get it into a real resource fork)" );
    fprintf( stderr, " -script [name|code]\tThe name should be the name of a mac script\n\t\tlike Roman or Cyrillic (not all names are recognized)\n\t\tthe code can be a number representing a script\n" );
    fprintf( stderr, "A list of bdf/ttf files all with the same family name will be merged into\n" );
    fprintf( stderr, "one FOND and all stored in one output files. Any pfb files will be refered\n" );
    fprintf( stderr, "to in the FOND but will live in a seperate file.\n" );
    exit( 1 );
}

static Face *ParseArgs(int argc, char **argv) {
    Face *head = NULL, *last=NULL, *cur;
    int i, bad, val;
    char *pt, *end;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    char *pt = argv[i]+1;
	    if ( *pt=='-' ) ++pt;		/* deal with double dashes too */
	    if ( strcmp(pt,"dfont")==0 )
		output_format = of_dfont;
	    else if ( strcmp(pt,"bin")==0 || strcmp(pt,"macbin")==0 )
		output_format = of_macbin;
	    else if ( strcmp(pt,"res")==0 || strcmp(pt,"resource")==0 )
		output_format = of_res;
	    else if ( strcmp(pt,"script")==0 ) {
		int bad = 0;
		if ( ++i >= argc )
		    bad=true;
		else if ( strcasecmp(argv[i],"roman")==0 )
		    script = 0;
		else if ( strcasecmp(argv[i],"japanese")==0 )
		    script = 1;
		else if ( strcasecmp(argv[i],"traditionalchinese")==0 ||
			  strcasecmp(argv[i],"tradchinese")==0 ||
			  strcasecmp(argv[i],"big5")==0 ||
			  strcasecmp(argv[i],"taiwan")==0 )
		    script = 2;
		else if ( strcasecmp(argv[i],"simplifiedchinese")==0 ||
			  strcasecmp(argv[i],"simpchinese")==0 )
		    script = 25;
		else if ( strcasecmp(argv[i],"korean")==0 )
		    script = 3;
		else if ( strcasecmp(argv[i],"arabic")==0 )
		    script = 4;
		else if ( strcasecmp(argv[i],"hebrew")==0 )
		    script = 5;
		else if ( strcasecmp(argv[i],"greek")==0 )
		    script = 6;
		else if ( strcasecmp(argv[i],"cyrillic")==0 ||
			  strcasecmp(argv[i],"russian")==0 )
		    script = 7;
		else if ( (val = strtol(argv[i],&end,10), *end!='\0') || val<0 || val>128 )
		    bad = true;
		else
		    script = val;
		if ( bad ) {
		    fprintf( stderr, "The script argument must be followed by a script name or number\n(Roman=0, Japanese=1, Big5=2, Korean=3, Arabic=4, Hebrew=5, Greek=6, Cyrillic=7)\n" );
		    exit(1);
		}
	    } else
		Usage(argv[0]);
	} else {
	    cur = calloc(1,sizeof(Face));
	    cur->filename = argv[i];
	    pt = strrchr(argv[i],'.');
	    cur->type = -1;
	    bad = 0;
	    if ( pt==NULL )
		/* do nothing */;
	    else if ( strcasecmp(pt,".bdf")==0 ) {
		cur->type = ft_bdf;
		bad = !BDFGetNames(cur);
	    } else if ( strcasecmp(pt,".ttf")==0 || strcasecmp(pt,".otf")==0 ) {
		cur->type = ft_ttf;
		bad = !TTFGetNames(cur);
	    } else if ( strcasecmp(pt,".pfb")==0 ) {
		cur->type = ft_ps;
		bad = !PSGetNames(cur);
	    }
	    /* I'm not going to support .pfa files, doing the conversion is */
	    /*  a pain, and I've already written something that does it. */
	    if ( cur->type==-1 ) {
		fprintf( stderr, "Unknown file type for %s\n Must be one of .bdf, .pfb, .ttf or .otf\n", argv[i] );
		Usage( argv[0] );
	    } else if ( bad )
		Usage( argv[0] );
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
    if ( head==NULL ) {
	fprintf( stderr, "No files\n" );
	Usage(argv[0]);
    }
return( head );
}

static Family *SortByFamily(Face *faces) {
    Family *head=NULL, *last=NULL, *cur;
    Face *next;
    int index;
    int i,cnt,one;

    for ( ; faces!=NULL ; faces = next ) {
	next = faces->next;
	/* mac styles may be numbers < 96. Postscript styles <48 */
	/* But we still might get two bitmaps with the same style>=48 so */
	/*  merge them into one FOND if they match */
	if ( faces->style<48 )
	    for ( cur=head; cur!=NULL && strcmp(cur->familyname,faces->family)!=0; cur = cur->next );
	else
	    for ( cur=head; cur!=NULL && strcmp(cur->familyname,faces->fontname)!=0; cur = cur->next );
	if ( cur==NULL ) {
	    cur = calloc(1,sizeof(Family));
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	    cur->familyname = faces->style<48 ? faces->family : faces->fontname;
	}
	if ( faces->type!=ft_ps ) cur->fixed = faces->fixed;
	index = faces->style;
	if ( index>=96 ) {
	    index = 0;
	    fprintf( stderr, "%s has a style which doesn't fit in a normal FOND, so we're\ngiving it its own FOND where it can pretend to be plain\n", faces->filename );
	}
	if ( faces->type==ft_bdf ) {
	    faces->next = cur->faces[index];
	    cur->faces[index] = faces;
	} else if ( faces->type==ft_ttf ) {
	    if ( cur->ttffaces[index]!=NULL )
		fprintf( stderr, "Attempt to add two scalable fonts for the same style in a FOND\n%s and %s\n", faces->filename, cur->ttffaces[index]->filename );
	    cur->ttffaces[index] = faces;
	    faces->next = NULL;
	} else {
	    index = faces->psstyle;
	    if ( index>=48 ) {
		index = 0;
		fprintf( stderr, "%s has a style which doesn't fit in a normal FOND, so we're\ngiving it its own FOND where it can pretend to be plain\n", faces->filename );
	    }
	    if ( cur->psfaces[index]!=NULL )
		fprintf( stderr, "Attempt to add two postscript fonts for the same style in a FOND\n%s and %s\n", faces->filename, cur->psfaces[index]->filename );
	    cur->psfaces[index] = faces;
	    faces->next = NULL;
	}
    }

    for ( cur=head; cur!=NULL; cur=cur->next ) {
	for ( i=0; i<48; ++i ) {
	    /* ps style is different from mac style, hence the complexity */
	    if ( cur->psfaces[i]!=NULL && cur->faces[cur->psfaces[i]->style]==NULL ) {
		fprintf( stderr, "A postscript font was found without a bitmap font of the same style\n %s\n", cur->psfaces[i]->filename );
		exit( 1 );
	    }
	}
	for ( i=cnt=0; i<96; ++i )
	    if ( cur->faces[i]!=NULL || cur->ttffaces[i]!=NULL ) {
		one = i;
		++cnt;
	    }
	if ( one!=0 && cnt==1 ) {
	    fprintf( stderr, "%s does not have a corresponding plain style font.\nWe will claim it is plain when we build the FOND\n",
		    cur->faces[one]!=NULL ? cur->faces[one]->filename:
					    cur->ttffaces[one]->filename );
	    cur->faces[0] = cur->faces[one];
	    cur->ttffaces[0] = cur->ttffaces[one];
	    cur->psfaces[0] = cur->psfaces[one];
	    cur->faces[one] = cur->ttffaces[one] = cur->psfaces[one] = NULL;
	    cur->familyname = cur->faces[one]!=NULL ? cur->faces[one]->fontname:
					    cur->ttffaces[one]->fontname;
	} else if ( cur->faces[0]==NULL && cur->ttffaces[0]==NULL ) {
	    fprintf( stderr, "The family, %s, does not have a plain style. That would lead to\nconfusion, so I'm giving up.\n", cur->familyname );
	    exit( 1 );
	}
	cur->id = HashToId(cur->familyname);
    }
return( head );
}

static void putpsstring(FILE *res,char *fontname) {
    putc(strlen(fontname),res);
    if ( *fontname ) {
	if ( islower(*fontname))
	    putc(toupper(*fontname),res);
	else
	    putc(*fontname,res);
	for ( ++fontname; *fontname; ++fontname )
	    putc(*fontname,res);
    }
}

static uint32 FamilyToFOND(FILE *res,Family *fam) {
    uint32 rlenpos = ftell(res), widoffpos, widoffloc, kernloc, styleloc, glyphloc, end;
    int i,j,cnt, scnt, strcnt, pscnt, maxw, size;
    Face *face, *test;
    int exact, badmatch;
    /* Fonds are generally marked system heap and sometimes purgeable (resource flags) */

    /* Use a ttf font if we've got it (metrics more accurate), else the */
    /*  biggest bitmap */
    face = fam->ttffaces[0];
    if ( face==NULL ) {
	face = fam->faces[0];
	for ( test = face; test!=NULL; test=test->next )
	    if ( test->size > face->size ) face = test;
    }
    maxw = 0;
    for ( i=0; i<256; ++i )
	if ( face->metrics[i]>maxw ) maxw = face->metrics[i];

    putlong(0,res);			/* Fill in length later */
    putshort(fam->fixed?0x9000:0x1000,res);
    putshort(fam->id,res);
    putshort(0,res);			/* First character */
    putshort(255,res);			/* Last character */
    putshort((short) ((face->ascent*(1<<12))/(face->ascent+face->descent)),res);
    putshort(-(short) ((face->descent*(1<<12))/(face->ascent+face->descent)),res);
    putshort((short) ((face->linegap*(1<<12))/(face->ascent+face->descent)),res);
    putshort((short) maxw,res);
    widoffpos = ftell(res);
    putlong(0,res);			/* Fill in width offset later */
    putlong(0,res);			/* Fill in kern offset later */
    putlong(0,res);			/* Fill in style offset later */
    for ( i=0; i<9; ++i )
	putshort(0,res);		/* Extra width values */
    putlong(0,res);			/* Script for international */
    putshort(2,res);		/* FOND version */

    /* Font association table */
    for ( i=cnt=scnt=0; i<96; ++i ) {
	for ( face = fam->faces[i]; face!=NULL; face = face->next )
	    ++cnt;
	if ( fam->ttffaces[i] )
	    ++cnt;
	if ( fam->faces[i]!=NULL || fam->ttffaces[i]!=NULL )
	    ++scnt;
    }
    putshort(cnt-1,res);		/* Number of faces */
    /* This list is ordered by size and then by style */
    for ( i=cnt=0; i<96; ++i ) if ( fam->ttffaces[i]!=NULL ) {
	putshort(0,res);		/* it's scaleable */
	putshort(i,res);		/* style */
	fam->ttffaces[i]->id = fam->id + cnt++;
	putshort(fam->ttffaces[i]->id,res);
    }
    for ( i=0; i<96; ++i )	/* I depend on this ordering */
	for ( face=fam->faces[i]; face!=NULL; face=face->next )
	    face->id = fam->id + cnt++;
    for ( size=1; size<256; ++size ) {
	for ( i=0; i<96; ++i ) {
	    for ( face=fam->faces[i]; face!=NULL; face=face->next ) if ( face->size==size ) {
		putshort(face->size,res);
		putshort(i,res);		/* style */
		putshort(face->id,res);		/* make up a unique ID */
	    }
	}
    }

    /* offset table */
    putshort(1-1,res);			/* One table */
    putlong(6,res);			/* Offset from start of otab to next byte */

    /* bounding box table */
    putshort(scnt-1,res);		/* One bounding box per style */
    for ( i=0; i<96; ++i ) if ( fam->faces[i]!=NULL || fam->ttffaces[i]!=NULL ) {
	/* Use a ttf font if we've got it (metrics more accurate), else the */
	/*  biggest bitmap */
	face = fam->ttffaces[i];
	if ( face==NULL ) {
	    face = fam->faces[i];
	    for ( test = face; test!=NULL; test=test->next )
		if ( test->size > face->size ) face = test;
	}
	putshort(i,res);		/* style */
	putshort(face->xmin,res);
	putshort(face->ymin,res);
	putshort(face->xmax,res);
	putshort(face->ymax,res);
    }

    widoffloc = ftell(res);
    putshort(scnt-1,res);		/* One set of width metrics per style */
    for ( i=0; i<96; ++i ) if ( fam->faces[i]!=NULL || fam->ttffaces[i]!=NULL ) {
	face = fam->ttffaces[i];
	if ( face==NULL ) {
	    face = fam->faces[i];
	    for ( test = face; test!=NULL; test=test->next )
		if ( test->size > face->size ) face = test;
	}
	putshort(i,res);		/* style */
	for ( j=0; j<256; ++j )
	    putshort(face->metrics[j],res);
	putshort(1<<12,res);	/* Seem to be two extra glyphs. default is one */
	putshort(1<<12,res);	/* 1 em is default size */
    }

    kernloc = 0;

    exact = badmatch = false;
    for ( i=pscnt=0; i<48; ++i ) if ( fam->psfaces[i]!=NULL ) {
	++pscnt;
	if ( strcmp(fam->familyname,fam->psfaces[i]->fontname)==0 )
	    exact = true;
	if ( strncmp(fam->psfaces[i]->fontname,fam->familyname,strlen(fam->familyname))!=0 )
	    badmatch = true;
    }
    styleloc = 0;
    if ( pscnt!=0 ) {
	char *family = badmatch ? "" : fam->familyname;
	int fontclass;
	if ( badmatch ) exact = false;
	styleloc = ftell(res);
	fontclass = 0x1;
	if ( fam->psfaces[psf_outline]==NULL ) fontclass |= 4;
	if ( fam->psfaces[psf_bold]!=NULL ) fontclass |= 0x18;
	if ( fam->psfaces[psf_italic]!=NULL ) fontclass |= 0x40;
	if ( fam->psfaces[psf_condense]!=NULL ) fontclass |= 0x80;
	if ( fam->psfaces[psf_extend]!=NULL ) fontclass |= 0x100;
	putshort(fontclass,res);	/* fontClass */
	putlong(0,res);			/* Offset to glyph encoding table (which we don't use) */
	putlong(0,res);			/* Reserved, MBZ */
	strcnt = 1/* Family Name */ + pscnt-exact /* count of format strings */ +
		pscnt-exact /* count of additional strings */;
	/* indeces to format strings */
	for ( i=0,pscnt=2; i<48; ++i )
	    if ( fam->psfaces[i]==NULL || strcmp(family,fam->psfaces[i]->fontname)==0)
		putc(1,res);
	    else
		putc(pscnt++,res);
	putshort(strcnt,res);		/* strcnt strings */
	putpsstring(res,family);
	/* Now the format strings */
	for ( i=0; i<48; ++i ) if ( fam->psfaces[i]!=NULL ) {
	    if ( strcmp(family,fam->psfaces[i]->fontname)!=0 ) {
		putc(1,res);		/* Familyname with the following */
		putc(pscnt++,res);
	    }
	}
	/* Now the additional names */
	for ( i=0; i<48; ++i ) if ( fam->psfaces[i]!=NULL ) {
	    if ( strcmp(family,fam->psfaces[i]->fontname)!=0 )
		putpsstring(res,fam->psfaces[i]->fontname+strlen(family));
	}
        /* Greg: record offset for glyph encoding table */
	/* We assume that the bitmap and postscript fonts are encoded similarly */
	/*  and so a null vector will do. */
        glyphloc = ftell( res );
        putshort(0, res); /* Greg: an empty Glyph encoding table */
    }

    end = ftell(res);
    fseek(res,widoffpos,SEEK_SET);
    putlong(widoffloc-rlenpos-4,res);	/* Fill in width offset */
    putlong(kernloc!=0?kernloc-rlenpos-4:0,res);	/* Fill in kern offset */
    putlong(styleloc!=0?styleloc-rlenpos-4:0,res);	/* Fill in style offset */

    /* Greg: go back and add the glyph encoding table offset */
    if (styleloc && glyphloc) {
        fseek(res, styleloc + 2, SEEK_SET);
        putlong(glyphloc-styleloc, res);
    }

    fseek(res,rlenpos,SEEK_SET);
    putlong(end-rlenpos-4,res);		/* resource length */
    fseek(res,end,SEEK_SET);
return(rlenpos);
}

/* I presume this routine is called after all resources have been written */
static void DumpResourceMap(FILE *res,struct resourcetype *rtypes) {
    uint32 rfork_base = output_format!=of_macbin?0:128;	/* space for mac binary header */
    uint32 resource_base = rfork_base+0x100;
    uint32 rend, rtypesstart, mend, namestart;
    int i,j;

    fseek(res,0,SEEK_END);
    rend = ftell(res);

    if ( output_format!=of_dfont ) {
	/* Duplicate resource header */
	putlong(0x100,res);			/* start of resource data */
	putlong(rend-rfork_base,res);		/* start of resource map */
	putlong(rend-rfork_base-0x100,res);	/* length of resource data */
	putlong(0,res);				/* don't know the length of the map section yet */
    } else {
	for ( i=0; i<16; ++i )			/* 16 bytes of zeroes */
	    putc(0,res);
    }

    putlong(0,res);			/* Some mac specific thing I don't understand */
    putshort(0,res);			/* another */
    putshort(0,res);			/* another */

    putshort(4+ftell(res)-rend,res);	/* Offset to resource types */
    putshort(0,res);			/* Don't know where the names go yet */

    rtypesstart = ftell(res);
    for ( i=0; rtypes[i].tag!=0; ++i );
    putshort(i-1,res);			/* Count of different types */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	putlong(rtypes[i].tag,res);	/* Resource type */
	putshort(0,res);		/* Number of resources of this type */
	putshort(0,res);		/* Offset to the resource list */
    }

    /* Now the resource lists... */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	rtypes[i].resloc = ftell(res);
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    putshort(rtypes[i].res[j].id,res);
	    rtypes[i].res[j].nameptloc = ftell(res);
	    putshort(0xffff,res);		/* assume no name at first */
	    putc(rtypes[i].res[j].flags,res);	/* resource flags */
		/* three byte resource offset */
	    putc( ((rtypes[i].res[j].pos-resource_base)>>16)&0xff, res );
	    putc( ((rtypes[i].res[j].pos-resource_base)>>8)&0xff, res );
	    putc( ((rtypes[i].res[j].pos-resource_base)&0xff), res );
	    putlong(0,res);
	}
    }
    namestart = ftell(res);
    /* Now the names, if any */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    if ( rtypes[i].res[j].name!=NULL ) {
		rtypes[i].res[j].nameloc = ftell(res);
		putc(strlen(rtypes[i].res[j].name),res);	/* Length */
		fwrite(rtypes[i].res[j].name,1,strlen(rtypes[i].res[j].name),res);
	    }
	}
    }
    mend = ftell(res);

    /* Repeat the rtypes list now we know where they go */
    fseek(res,rtypesstart+2,SEEK_SET);		/* skip over the count */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	putlong(rtypes[i].tag,res);	/* Resource type */
	for ( j=0; rtypes[i].res[j].pos!=0; ++j );
	putshort(j-1,res);		/* Number of resources of this type */
	putshort(rtypes[i].resloc-rtypesstart,res);
    }
    /* And go back and fixup any name pointers */
    for ( i=0; rtypes[i].tag!=0; ++i ) {
	for ( j=0; rtypes[i].res[j].pos!=0; ++j ) {
	    if ( rtypes[i].res[j].name!=NULL ) {
		fseek(res,rtypes[i].res[j].nameptloc,SEEK_SET);
		putshort(rtypes[i].res[j].nameloc-namestart,res);
	    }
	}
    }

    fseek(res,rend,SEEK_SET);
    	/* Fixup duplicate header (and offset to the name list) */
    if ( output_format!=of_dfont ) {
	putlong(0x100,res);			/* start of resource data */
	putlong(rend-rfork_base,res);		/* start of resource map */
	putlong(rend-rfork_base-0x100,res);	/* length of resource data */
	putlong(mend-rend,res);			/* length of map section */
    } else {
	for ( i=0; i<16; ++i )
	    putc(0,res);
    }

    putlong(0,res);			/* Some mac specific thing I don't understand */
    putshort(0,res);			/* another */
    putshort(0,res);			/* another */

    putshort(4+ftell(res)-rend,res);	/* Offset to resource types */
    putshort(namestart-rend,res);	/* name section */

    fseek(res,rfork_base,SEEK_SET);
    	/* Fixup main resource header */
    putlong(0x100,res);			/* start of resource data */
    putlong(rend-rfork_base,res);	/* start of resource map */
    putlong(rend-rfork_base-0x100,res);	/* length of resource data */
    putlong(mend-rend,res);		/* length of map section */
}

/* A MacBinary file */
/*  http://www.lazerware.com/formats/macbinary.html */
/*    begins with a 128 byte header */
/*	(which specifies lengths for data/resource forks) */
/*	(and contains mac type/creator data) */
/*	(and other stuff) */
/*	(and finally a crc checksum)
/*    is followed by the data section (padded to a mult of 128 bytes) */
/*    is followed by the resource section (padded to a mult of 128 bytes) */

/* Crc code taken from: */
/* http://www.ctan.org/tex-archive/tools/macutils/crc/ */
/* MacBinary files use the same CRC that binhex does (in the MacBinary header) */
extern unsigned long binhex_crc(unsigned char *buffer,int size);

static void DumpMacBinaryHeader(FILE *res,struct macbinaryheader *mb) {
    uint8 header[128], *hpt; char buffer[256], *pt, *dpt;
    uint32 len;
    time_t now;
    int i,crc;

    if ( mb->macfilename==NULL ) {
	char *pt = strrchr(mb->binfilename,'/');
	if ( pt==NULL ) pt = mb->binfilename;
	else ++pt;
	strcpy(buffer,pt);
	dpt = strrchr(buffer,'.');
	if ( dpt==NULL ) {
	    buffer[0] = '_';
	    strcpy(buffer+1,pt);
	} else
	    *dpt = '\0';
	mb->macfilename = buffer;
	buffer[63] = '\0';
    }

    memset(header,'\0',sizeof(header));
    hpt = header;
    *hpt++ = '\0';		/* version number */
    /* Mac Filename */
    pt = mb->macfilename;
    *hpt++ = strlen( pt );
    while ( *pt )
	*hpt++ = *pt++;
    while ( hpt<header+65 )
	*hpt++ = '\0';
    /* Mac File Type */
    *hpt++ = mb->type>>24; *hpt++ = mb->type>>16; *hpt++ = mb->type>>8; *hpt++ = mb->type;
    /* Mac Creator */
    *hpt++ = mb->creator>>24; *hpt++ = mb->creator>>16; *hpt++ = mb->creator>>8; *hpt++ = mb->creator;
    *hpt++ = '\0';		/* No finder flags set */
    *hpt++ = '\0';		/* (byte 74) MBZ */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Vert Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Hor Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* window or folder id??? */
    *hpt++ = '\0';		/* protected bit ??? */
    *hpt++ = '\0';		/* (byte 82) MBZ */
	/* Data fork length */
    *hpt++ = '\0'; *hpt++ = '\0'; *hpt++ = '\0'; *hpt++ = '\0';
	/* Resource fork length */
    fseek(res,0,SEEK_END);
    len = ftell(res)-sizeof(header);
    *hpt++ = len>>24; *hpt++ = len>>16; *hpt++ = len>>8; *hpt++ = len;
	/* Pad resource fork to be a multiple of 128 bytes */
    while ( (len&127)!=0 )
	{ putc('\0',res); ++len; }

	/* Creation time, (seconds from 1/1/1904) */
    time(&now);
    /* convert from 1970 based time to 1904 based time */
    now += (1970-1904)*365L*24*60*60;
    for ( i=1904; i<1970; i+=4 )
	now += 24*60*60;
    /* Ignore any leap seconds */
    *hpt++ = now>>24; *hpt++ = now>>16; *hpt++ = now>>8; *hpt++ = now;
	/* Modification time, (seconds from 1/1/1904) */
    *hpt++ = now>>24; *hpt++ = now>>16; *hpt++ = now>>8; *hpt++ = now;

    *hpt++ = '\0'; *hpt++ = '\0';	/* Get Info comment length */
    *hpt++ = 0;				/* More finder flags */

/* MacBinary 3 */
    memcpy(header+102,"mBIN",4);
    header[106] = 0;			/* Script. I assume 0 is latin */
    header[107] = 0;			/* extended finder flags */
/* End of MacBinary 3 */
    header[122] = 130;			/* MacBinary version 3, written in (129 is MB2) */
    header[123] = 129;			/* MacBinary Version 2, needed to read */

    crc = binhex_crc(header,124);
    header[124] = crc>>8;
    header[125] = crc;

    fseek(res,0,SEEK_SET);
    fwrite(header,1,sizeof(header),res);
}

static void WriteDummyHeaders(FILE *res) {
    /* Leave space for the mac binary header (128bytes) and the mac resource */
    /*  file header (256 bytes) */
    int i;
    if ( output_format == of_macbin ) {
	for ( i=0; i<128; ++i )
	    putc(0,res);
    }
    for ( i=0; i<256; ++i )
	putc(0,res);
}

static int DumpPostscriptFont(Face *face) {
    FILE *res;
    int ret = 1;
    struct resourcetype resources[2];
    struct macbinaryheader header;
    char buffer[63], *pt, *spt, *lcpt=NULL;
    char filename[63], *fpt;

    fpt = filename;
    for ( pt = buffer, spt = face->fontname; *spt && pt<buffer+sizeof(buffer)-1; ++spt ) {
	if ( isupper(*spt)) {
	    *pt++ = *spt;
	    lcpt = (spt==face->fontname?spt+5:spt+3);
	} else if ( islower(*spt) && spt<lcpt )
	    *pt++ = *spt;
	if ( isalnum(*spt) && fpt<filename+sizeof(filename)-6 )
	    *fpt++ = *spt;
    }
    *pt = '\0'; *fpt = '\0';

    if ( output_format!=of_macbin )
	strcpy(filename,buffer);
    else
	strcat(filename,".bin");
    res = fopen(filename,"w+");
    if ( res==NULL )
return( 0 );

    WriteDummyHeaders(res);
    memset(resources,'\0',sizeof(resources));

    resources[0].tag = CHR('P','O','S','T');
    resources[0].res = PSToResources(res,face);
    if ( resources[0].res==NULL ) {
	fclose(res);
	unlink(filename);
return( 0 );
    }
    DumpResourceMap(res,resources);
    free( resources[0].res );

    if ( output_format==of_macbin ) {
	header.macfilename = buffer;
	    /* Adobe uses a creator of ASPF (Adobe Systems Postscript Font I assume) */
	    /* Fontographer uses ACp1 (Altsys Corp. Postscript type 1???) */
	    /* Both include an FREF, BNDL, ICON* and comment */
	    /* I shan't bother with that... It'll look ugly with no icon, but oh well */
	header.type = CHR('L','W','F','N');
	header.creator = CHR('G','W','p','1');
	DumpMacBinaryHeader(res,&header);
    }
    ret = !ferror(res);
    if ( fclose(res)==-1 ) ret = 0;
return( ret );
}

static void DumpFamily(Family *fam) {
    int i,ncnt, tcnt, ret;
    int npos, tpos, fpos;
    Face *face;
    struct resourcetype resources[4];
    struct resource fonds[2];
    FILE *res;
    struct macbinaryheader header;
    char filename[256];

    for ( i=0; i<48; ++i )
	if ( fam->psfaces[i]!=NULL )
	    if ( !DumpPostscriptFont(fam->psfaces[i]) )
		fprintf( stderr, "Failed to write resource file for PostScript %s\n", face->fontname );

    for ( i=ncnt=tcnt=0; i<96; ++i ) {
	for ( face=fam->faces[i]; face!=NULL; face=face->next )
	    ++ncnt;
	if ( fam->ttffaces[i]!=NULL )
	    ++tcnt;
    }

    if ( ncnt==0 && tcnt==0 )
return;

    strcpy(filename,fam->familyname);
    strcat(filename,".fam");
    strcat(filename,output_format==of_dfont?".dfont":
		    output_format==of_macbin?".bin":
		    ".rsrc");
    res = fopen(filename,"w");
    if ( res==NULL ) {
	fprintf( stderr, "Failed to open output file %s\n", filename );
return;
    }
    WriteDummyHeaders(res);

    memset( resources, 0, sizeof(resources));
    memset( fonds, 0, sizeof(fonds));
    npos = tpos = fpos = 0;
    if ( ncnt!=0 ) {
	resources[0].tag = CHR('N','F','N','T');
	resources[0].res = calloc(ncnt+1,sizeof(struct resource));
	for ( i=ncnt=tcnt=0; i<96; ++i ) {
	    for ( face=fam->faces[i]; face!=NULL; face=face->next ) {
		resources[0].res[ncnt].pos = BDFToResource(res,face);
		resources[0].res[ncnt].flags = 0x00;	/* NFNTs generally have flags of 0 */
		resources[0].res[ncnt].id = fam->id+ncnt;
		++ncnt;
	    }
	}
	tpos = fpos = 1;
    }
    if ( tcnt!=0 ) {
	resources[tpos].tag = CHR('s','f','n','t');
	resources[tpos].res = calloc(tcnt+1,sizeof(struct resource));
	for ( i=tcnt=0; i<96; ++i ) {
	    if ( fam->ttffaces[i]!=NULL ) {
		resources[tpos].res[tcnt].pos = TTFToResource(res,fam->ttffaces[i]);
		resources[tpos].res[tcnt].flags = 0x00;	/* sfnts generally have resource flags 0x20 */
		resources[tpos].res[tcnt].id = fam->id+tcnt;
		++tcnt;
	    }
	}
	++fpos;
    }

    resources[fpos].tag = CHR('F','O','N','D');
    resources[fpos].res = fonds;
    fonds[0].pos = FamilyToFOND(res,fam);
    fonds[0].flags = 0x00;	/* I've seen FONDs with resource flags 0, 0x20, 0x60 */
    fonds[0].id = fam->id;
    fonds[0].name = fam->familyname;

    DumpResourceMap(res,resources);

    if ( output_format==of_macbin ) {
	header.macfilename = NULL;
	header.binfilename = filename;
	    /* Fontographer uses the old suitcase format for both bitmaps and ttf */
	header.type = CHR('F','F','I','L');
	header.creator = CHR('D','M','O','V');
	DumpMacBinaryHeader(res,&header);
    }
    ret = !ferror(res);
    if ( fclose(res)==-1 || ret == 0 )
	fprintf( stderr, "Failed to write resource file for family %s\n", fam->familyname );
}

int main( int argc, char **argv) {
    Family *families;

    families = SortByFamily(ParseArgs(argc, argv));
    for ( ; families!=NULL; families=families->next )
	DumpFamily(families);

return( 0 );
}
