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

/* Process postscript files to make them useable on the mac */
/* this involves almost no work, because all the hard stuff is done on the bitmap */

static long getpfblong(FILE *f) {
    int ch1 = getc(f);
    int ch2 = getc(f);
    int ch3 = getc(f);
    int ch4 = getc(f);
    if ( ch4==EOF )
return( EOF );
return( ch4|(ch3<<8)|(ch2<<16)|(ch1<<24) );
}

int PSGetNames(Face *face) {
    /* Get the font and family names by reading the fontfile */
    FILE *pf = fopen( face->filename, "r" );
    char buffer[512];
    int ch;
    char *pt, *npt;

    if ( pf==NULL ) {
	fprintf( stderr, "Can't open %s for reading\n", face->filename );
return(false);
    }
    ch = getc(pf);
    if ( ch==0x80 ) { /* skip over the pfb header */
	getc(pf); getc(pf); getc(pf); getc(pf); getc(pf);
    } else {
	ungetc(ch,pf);
	fprintf( stderr, "Hmm, %s doesn't look like a pfb font,\nif it's a pfa just convert it to pfb and try again.\n", face->filename );
	fclose(pf);
return( false );
    }
    buffer[0]='\0';
    fgets(buffer,sizeof(buffer),pf);
    if ( buffer[0]!='%' || buffer[1]!='!' ) {
	fprintf( stderr, "%s does not look like a postscript font\n", face->fontname);
	fclose(pf);
return( false );
    }

    while ( fgets(buffer,sizeof(buffer),pf)!=NULL ) {
	if ( (pt=strstr(buffer,"/FamilyName"))!=NULL ) {
	    pt += strlen("/FamilyName");
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='(' ) ++pt;
	    /* don't deal with the full complexities of strings, just look for*/
	    /*  the final ) and assume we've got normal chars, no nested parens*/
	    for ( npt=pt; *npt!=')' && *npt!='\0'; ++npt );
	    *npt = '\0';
	    face->family = strdup(pt);
	    if ( face->fontname!=NULL )
    break;
	} else if ( (pt=strstr(buffer,"/FontName"))!=NULL ) {
	    pt += strlen("/FontName");
	    while ( *pt==' ' ) ++pt;
	    if ( *pt=='/' ) ++pt;
	    for ( npt=pt; *npt!=' ' && *npt!='\0'; ++npt );
	    *npt = '\0';
	    face->fontname = strdup(pt);
	    if ( face->family!=NULL )
    break;
	}
    }
    fclose(pf);
    if ( face->fontname==NULL && face->family!=NULL )
	face->fontname = strdup(face->family);
    if ( face->fontname!=NULL ) {
	if ( strstr(face->fontname,"Bold")!=NULL ) {
	    face->style |= sf_bold;
	    face->psstyle |= psf_bold;
	}
	if ( strstr(face->fontname,"Italic")!=NULL || strstr(face->fontname,"Oblique")!=NULL ) {
	    face->style |= sf_italic;
	    face->psstyle |= psf_italic;
	}
	if ( strstr(face->fontname,"Outline")!=NULL ) {
	    face->style |= sf_outline;
	    face->psstyle |= psf_outline;
	}
	if ( strstr(face->fontname,"Shadow")!=NULL ) {
	    face->style |= sf_shadow;
	    face->psstyle |= psf_shadow;
	}
	if ( strstr(face->fontname,"Condense")!=NULL ) {
	    face->style |= sf_condense;
	    face->psstyle |= psf_condense;
	}
	if ( strstr(face->fontname,"Extend")!=NULL ) {
	    face->style |= sf_extend;
	    face->psstyle |= psf_extend;
	}
    }
return( face->fontname!=NULL );
}
	    
struct resource *PSToResources(FILE *res,Face *face) {
    /* split the font up into as many small resources as we need and return */
    /*  an array pointing to the start of each */
    struct stat statb;
    int cnt, type, len, i;
    struct resource *resstarts;
    FILE *pf;

    stat(face->filename,&statb);
    cnt = 3*(statb.st_size+0x800)/(0x800-2)+1;		/* should be (usually) a vast over estimate */
    resstarts = calloc(cnt+1,sizeof(struct resource));

    pf = fopen(face->filename,"r");
    cnt = 0;
    forever {
	if ( getc(pf)!=0x80 ) {
	    fprintf( stderr, "Missing pfb section head in %s\n", face->fontname );
	    fclose(pf);
return( NULL );
	}
	type = getc(pf);
	if ( type==3 ) {
	    resstarts[cnt].id = 501+cnt;	/* 501 appears to be magic */
	    resstarts[cnt++].pos = ftell(res);
	    putlong(2,res);	/* length */
	    putc(5,res);	/* eof mark */
	    putc(0,res);
    break;
	}
	len = getpfblong(pf);
	while ( len>0 ) {
	    int ilen = len;
	    if ( ilen>0x800-2 )
		ilen = 0x800-2;
	    len -= ilen;
	    resstarts[cnt].id = 501+cnt;
	    resstarts[cnt++].pos = ftell(res);
	    putlong(ilen+2,res);	/* length */
	    putc(type,res);		/* section type mark */
	    putc(0,res);
	    for ( i=0; i<ilen; ++i )
		putc(getc(pf),res);
	}
    }
    fclose(pf);
    resstarts[cnt].pos = 0;
return( resstarts );
}
