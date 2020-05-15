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
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "macfonts.h"

int tolatin1 = false;
static int force = false, inquire = false, doafm = false, trackps = false, show=false;

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

static int _cleanfilename(char *filename) {
    char *pt, *npt;
    int ch, exists, ch2;

    for ( ch = *(pt=npt=filename); ch!='\0'; ch = *++pt ) {
	if ( ch>'!' && ch!='*' && ch!='?' && ch!='/' && ch!='\\' && ch!='[' && ch<0x7f )
	    *npt++ = ch;
    }
    *npt = '\0';
    if ( force && !inquire )
return( true );

    exists = access(filename,F_OK)==0;

    forever {
	if ( exists )
	    fprintf( stderr, "%s exists, do you want to overwrite it? (n) ", filename );
	else if ( inquire )
	    fprintf( stderr, "Write %s? (y) ", filename );
	else
return( true );
	ch = getchar();
	if ( ch=='q' || ch=='Q' )
exit(0);
	if ( ch=='a' || ch=='A' ) {
	    force = true;
	    inquire = false;
return( true );
	}
	if ( ch=='y' || ch=='Y' || ch=='n' || ch=='N' || ch==EOF || ch=='\n' ) {
	    if ( ch=='\n' || ch==EOF )
		ch = exists ? 'n' : 'y';
	    else
		while ( (ch2=getchar())!='\n' && ch2!=EOF );
return( ch=='y' || ch=='Y' );
	} else if ( ch=='=' ) {
	    for ( pt=filename; (ch=getchar())!=EOF && ch!='\n'; )
		*pt++ = ch;
return( true );
	} else {
	    fprintf( stderr, "Please answer with 'y'(es), 'n'(o), 'q'(uit), 'a'(ll), or '=new-filename'.\n" );
	}
    }
}

int cleanfilename(char *filename) {
    int ret = _cleanfilename(filename);
    if ( ret && show )
	fprintf( stderr, "Creating %s\n", filename );
return( ret );
}

static void mytmpname(char *temp) {
    static int upos;
    /* build up a temporary filename that doesn't match anything else */

    forever {
	sprintf( temp, "fondu%04X-%d", getpid(), ++upos );
	if ( access(temp,F_OK)==-1 )
return;
    }
}

/* The mac has rules about what the filename should be for a postscript*/
/*  font. If you deviate from those rules the font will not be found */
/*  The font name must begin with a capital letter */
/*  The filename is designed by modifying the font name */
/*  After the initial capital there can be at most 4 lower case letters (or digits) */
/*   in the filename, any additional lc letters (or digits) in the fontname are ignored */
/*  Every subsequent capital will be followed by at most 2 lc letters */
/*  special characters ("-$", etc.) are removed entirely */
/* So Times-Bold => TimesBol, HelveticaDemiBold => HelveDemBol */
static char *FileNameFromPSFontName(char *fontname) {
    char *filename = strdup(fontname);
    char *pt, *spt, *lcpt;

    for ( pt = filename, spt = fontname; *spt /*&& pt<filename+63-1*/; ++spt ) {
	if ( isupper(*spt) || spt==fontname ) {
	    *pt++ = *spt;
	    lcpt = (spt==fontname?spt+5:spt+3);
	} else if ( (islower(*spt) || isdigit(*spt)) && spt<lcpt )
	    *pt++ = *spt;
    }
    *pt = '\0';
return( filename );
}

static int IsResourceInFile(char *filename,PSFONT *psfont);

static void ProcessNestedPS( char *fontname, char *origfilename, PSFONT *psfont ) {
    char *filename = FileNameFromPSFontName(fontname);

    if ( filename!=NULL && *filename!='\0' ) {
	char *dirend = strrchr(origfilename,'/');
	if ( dirend!=NULL ) {
        size_t newfnlen = strlen(filename)+strlen(origfilename)+1;
	    char *newfn = malloc(newfnlen);
	    strncpy(newfn,origfilename, newfnlen);
	    strncpy(newfn + (dirend-origfilename)+1, filename, strlen(filename)); /* we know we have malloced at least filename long?? */
	    free(filename);
	    filename = newfn;
	}
	if ( access(filename,R_OK)==0 )
	    IsResourceInFile(filename,psfont);
	else {
	    char *end;
	    filename = realloc(filename,strlen(filename)+10);
	    end = filename+strlen(filename);
	    strcpy(end,".bin");
	    if ( access(filename,R_OK)==0 )
		IsResourceInFile(filename,psfont);
	    else {
		strcpy(end,".hqx");
		if ( access(filename,R_OK)==0 )
		    IsResourceInFile(filename,psfont);
	    }
	}
    }
    if ( psfont!=NULL && psfont->fontname==NULL ) {
	char *pt = strrchr(filename,'.');
	char *ept = strrchr(filename,'/');
	if ( pt!=NULL && pt>ept ) *pt = '\0';
	fprintf( stderr, "Failed to find file %s when searching for a postscript resource\n", filename );
    }
    free(filename);
}

static FILE *CreateAfmFile(FILE *f,FOND *fond,int style,
	char *fontname,char *familyname, PSFONT *psfont, char *origfilename) {
    char namebuf[300];
    int i;
    FILE *afm;

    memset(psfont,0,sizeof(PSFONT));
    if ( !(style&4) && fond->psnames[(style&3)|((style&~7)>>1)]!=NULL ) {
	/* postscript name table doesn't include underline styles */
	char *fn = fond->psnames[(style&3)|((style&~7)>>1)];
	strcpy(familyname,fond->family);
	strcpy(fontname,fn);
	ProcessNestedPS( fn, origfilename, psfont );
	if ( psfont->familyname!=NULL )
	    strcpy(familyname,psfont->familyname);	/* Different in URW fonts */
    } else {
	if ( fond->family!=NULL )
	    strcpy(familyname,fond->family);
	else if ( fond->fondname!=NULL )
	    strcpy(familyname,fond->fondname);
	else
	    strcpy(familyname,"Nameless");
	strcpy(fontname,familyname);
	for ( i=0; styles[i]!=NULL ; ++i )
	    if ( style&(1<<i))
		strcat(fontname,styles[i]);
    }
    strcpy(namebuf,fontname);
    strcat(namebuf,".afm");
    if ( !cleanfilename(namebuf))
return( NULL );
    afm = fopen( namebuf,"w" );
    if ( afm==NULL )
	fprintf( stderr, "Can't open %s\n", namebuf );
return( afm );
}

static void AfmBB(FILE *afm, struct bbglyph *bb,int enc, double em) {
    fprintf( afm, "C %d ; WX %d ; ", enc, (int) (bb->hadvance*1000/em) );
    fprintf( afm, "N %s ; B %d %d %d %d ;",
	    bb->glyphname,
	    (int) floor(bb->left*1000/em), (int) floor(bb->bottom*1000/em),
	    (int) ceil(bb->right*1000/em), (int) ceil(bb->top*1000/em) );
    if ( strcmp(bb->glyphname,"fi")==0 )
	fprintf( afm, " L f i ;" );
    else if ( strcmp(bb->glyphname,"fl")==0 )
	fprintf( afm, " L f l ;" );
    else if ( strcmp(bb->glyphname,"ff")==0 )
	fprintf( afm, " L f f ;" );
    putc('\n',afm);
}

static void MakeAfmFiles(FOND *fond,FILE *f, int isfixed,char *origfilename) {
    long start = ftell(f);
    int ii,i,j,k,l,dups;
    FILE *afm;
    time_t now;
    char fontname[256], familyname[256];
    char buffer[32];
    PSFONT psfont;

    for ( ii=0; ii<48; ++ii ) {
	int style = ((ii&3)|((ii&~3)<<1));
	for ( i=fond->stylekerncnt-1; i>=0; --i )
	    if ( fond->stylekerns[i].style==style )
	break;
	if ( i==-1 && fond->psnames[ii]==NULL )
    continue;
	afm = CreateAfmFile(f,fond,style,
		fontname,familyname,&psfont,origfilename);
	if ( afm==NULL )
    continue;
	fprintf( afm, "StartFontMetrics 2.0\n" );
	if ( psfont.fontname==NULL )
	    fprintf( afm, "Comment Caveat: This does not contain all the information generally found in an AFM file\n" );
	fprintf( afm, "Comment Generated by Fondu from a mac FOND resource\n" );
	time(&now);
	fprintf( afm, "Comment Creation Date: %s", ctime(&now) );
	fprintf( afm, "FontName %s\n", fontname );
	fprintf( afm, "FamilyName %s\n", familyname );
	fprintf( afm, "IsFixedPitch %s\n", isfixed?"true":"false");
	if ( psfont.fontname!=NULL ) {
	    if ( psfont.fullname )
		fprintf( afm, "FullName %s\n", psfont.fullname );
	    if ( psfont.weight )
		fprintf( afm, "Weight %s\n", psfont.weight );
	    if ( psfont.notice )
		fprintf( afm, "Notice (%s)\n", psfont.notice );
	    if ( psfont.version )
		fprintf( afm, "Version (%s)\n", psfont.version );
	    fprintf( afm, "ItalicAngle %g\n", psfont.italicangle );
	    fprintf( afm, "EncodingScheme %s\n", psfont.isadobestd ?
		    "AdobeStandardEncoding" : "FontSpecific" );
	    fprintf( afm, "FontBBox %g %g %g %g\n",
		    psfont.fbb[0], psfont.fbb[1], psfont.fbb[2], psfont.fbb[3] );
	    if ( psfont.xh!=0 )
		fprintf( afm, "XHeight %d\n", psfont.xh );
	    if ( psfont.ch!=0 )
		fprintf( afm, "CapHeight %d\n", psfont.ch );
	    if ( psfont.as!=0 )
		fprintf( afm, "Ascender %d\n", psfont.as );
	    if ( psfont.ds!=0 )
		fprintf( afm, "Descender %d\n", psfont.ds );
	    dups = 0;
	    for ( k=0; k<256; ++k ) {
		if ( strcmp(psfont.glyphs[psfont.encoding[k]].glyphname,".notdef")==0 && psfont.glyphs[0].isref )
	    continue;
		for ( l=psfont.glyphcnt-1; l>=0; --l )
		    if ( strcmp(psfont.glyphs[psfont.encoding[k]].glyphname,psfont.glyphs[l].glyphname)==0 )
		break;
		if ( l==-1 )
	    continue;
		else if ( !psfont.glyphs[l].isref )
		    psfont.glyphs[l].isref = true;
		else
		    ++dups;
	    }
	    fprintf( afm, "StartCharMetrics %d\n", psfont.glyphcnt+dups );
	    psfont.glyphs[0].isref = false;
	    for ( k=0; k<256; ++k ) {
		if ( strcmp(psfont.glyphs[psfont.encoding[k]].glyphname,".notdef")==0 && psfont.glyphs[0].isref )
	    continue;
		for ( l=psfont.glyphcnt-1; l>=0; --l )
		    if ( strcmp(psfont.glyphs[psfont.encoding[k]].glyphname,psfont.glyphs[l].glyphname)==0 )
		break;
		if ( l==-1 )
	    continue;
		AfmBB(afm,&psfont.glyphs[l],k,psfont.em);
		psfont.glyphs[l].isref = true;
	    }
	    for ( l=0; l<psfont.glyphcnt; ++l ) if ( !psfont.glyphs[l].isref )
		AfmBB(afm,&psfont.glyphs[l],-1,psfont.em);
	} else {
	    fprintf( afm, "Weight %s\n",  ( style&sf_bold )?"Bold":"Medium" );
	    if ( !( style&sf_italic ))
		fprintf( afm, "ItalicAngle 0\n" );
	    fprintf( afm, "EncodingScheme FontSpecific\n" );
	    /* I could check to see if there's an offset table, and check it to */
	    /*  see if there's a bounding box table and then read that in and */
	    /*  convert it and output a proper FontBBox. But it might not be there*/
	    /*  and it's a pain, and pfaedit never looks at it anyway */
	    for ( j=fond->stylewidthcnt-1; j>=0; --j )
		if ( style==fond->stylewidths[j].style )
	    break;
	    if ( j!=-1 ) {
		fprintf( afm, "StartCharMetrics %d\n", fond->last-fond->first+1 );
		for ( k=fond->first; k<=fond->last; ++k ) {
		    const char *name;
		    if ( k<=256 && macnames[k]!=NULL )
			name = macnames[k];
		    else {
			sprintf( buffer, "char%04x", k );
			name = buffer;
		    }
		    fprintf( afm, "C %d ; WX %d ; N %s ;\n", 	/* No bounding box info */
			k>=256?-1:k,
			(fond->stylewidths[j].widthtab[k-fond->first]*1000 + (1<<11))>>12,
			name );
		}
	    } else 
		fprintf( afm, "StartCharMetrics 0\n" );
	}
	fprintf( afm, "EndCharMetrics\n" );
	if ( i!=-1 ) {
	    /* kerning data are from the fond. so they are in mac encoding */
	    /*  the pfb's encoding is irrelevant in this section */
	    fprintf( afm, "StartKernData\nStartKernPairs %d\n",
		    fond->stylekerns[i].kernpairs );
	    for ( k=0; k<fond->stylekerns[i].kernpairs; ++k ) {
		const char *name1, *name2;
		struct kerns *kp = &fond->stylekerns[i].kerns[k];
		if ( 1 /* kp->ch1<=256 */ )
		    name1 = macnames[kp->ch1];
		else {
		    sprintf( buffer, "char%04x", kp->ch1 );
		    name1 = buffer;
		}
		if ( name1==NULL ) name1 = ".notdef";
		if ( 1 /* kp->ch2<=256 */ )
		    name2 = macnames[kp->ch2];
		else {
		    sprintf( fontname, "char%04x", kp->ch2 );
		    name2 = fontname;
		}
		if ( name2==NULL ) name2 = ".notdef";
		fprintf( afm, "KPX %s %s %d\n", name1, name2, (kp->offset*1000+(1<<11))>>12 );
	    }
	    fprintf( afm, "EndKernPairs\nEndKernData\n");
	}
	fprintf( afm, "EndFontMetrics\n" );
	fclose( afm );
    }
    fseek(f,start,SEEK_SET);
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
	long name_list,char *origfilename) {
    long here, start = ftell(f);
    long offset;
    int rname = -1;
    char name[300];
    int ch1, ch2;
    int i, j, k, cnt, isfixed;
    FOND *head=NULL, *cur;
    long widoff, kernoff, styleoff;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	rname = (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	offset = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);

	cur = calloc(1,sizeof(FOND));
	cur->next = head;
	head = cur;

	if ( rname!=-1 ) {
	    fseek(f,name_list+rname,SEEK_SET);
	    ch1 = getc(f);
	    fread(name,1,ch1,f);
	    name[ch1] = '\0';
	    cur->fondname = strdup(name);
	}

	offset += 4;
	fseek(f,offset,SEEK_SET);
	isfixed = getushort(f)&0x8000?1:0;
	/* family id = */ getushort(f);
	cur->first = getushort(f);
	cur->last = getushort(f);
/* on a 1 point font... */
	/* ascent = */ getushort(f);
	/* descent = */ (short) getushort(f);
	/* leading = */ getushort(f);
	/* widmax = */ getushort(f);
	if ( (widoff = getlong(f))!=0 ) widoff += offset;
	if ( (kernoff = getlong(f))!=0 ) kernoff += offset;
	if ( (styleoff = getlong(f))!=0 ) styleoff += offset;
	for ( j=0; j<9; ++j )
	    getushort(f);
	/* internal & undefined, for international scripts = */ getlong(f);
	/* version = */ getushort(f);
	cur->assoc_cnt = getushort(f)+1;
	cur->assoc = calloc(cur->assoc_cnt,sizeof(struct assoc));
	for ( j=0; j<cur->assoc_cnt; ++j ) {
	    cur->assoc[j].size = getushort(f);
	    cur->assoc[j].style = getushort(f);
	    cur->assoc[j].id = getushort(f);
	}
	if ( widoff!=0 ) {
	    fseek(f,widoff,SEEK_SET);
	    cnt = getushort(f)+1;
	    cur->stylewidthcnt = cnt;
	    cur->stylewidths = calloc(cnt,sizeof(struct stylewidths));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylewidths[j].style = getushort(f);
		cur->stylewidths[j].widthtab = malloc((cur->last-cur->first+3)*sizeof(short));
		for ( k=cur->first; k<=cur->last+2; ++k )
		    cur->stylewidths[j].widthtab[k] = getushort(f);
	    }
	}
	if ( kernoff!=0 ) {
	    fseek(f,kernoff,SEEK_SET);
	    cnt = getushort(f)+1;
	    cur->stylekerncnt = cnt;
	    cur->stylekerns = calloc(cnt,sizeof(struct stylekerns));
	    for ( j=0; j<cnt; ++j ) {
		cur->stylekerns[j].style = getushort(f);
		cur->stylekerns[j].kernpairs = getushort(f);
		cur->stylekerns[j].kerns = malloc(cur->stylekerns[j].kernpairs*sizeof(struct kerns));
		for ( k=0; k<cur->stylekerns[j].kernpairs; ++k ) {
		    cur->stylekerns[j].kerns[k].ch1 = getc(f);
		    cur->stylekerns[j].kerns[k].ch2 = getc(f);
		    cur->stylekerns[j].kerns[k].offset = getushort(f);
		}
	    }
	}
	if ( styleoff!=0 ) {
	    unsigned char stringoffsets[48];
	    int strcnt, strlen, format;
	    char **strings, *pt;
	    fseek(f,styleoff,SEEK_SET);
	    /* class = */ getushort(f);
	    /* glyph encoding offset = */ getlong(f);
	    /* reserved = */ getlong(f);
	    for ( j=0; j<48; ++j )
		stringoffsets[j] = getc(f);
	    strcnt = getushort(f);
	    strings = malloc(strcnt*sizeof(char *));
	    for ( j=0; j<strcnt; ++j ) {
		strlen = getc(f);
		strings[j] = malloc(strlen+2);
		strings[j][0] = strlen;
		strings[j][strlen+1] = '\0';
		for ( k=0; k<strlen; ++k )
		    strings[j][k+1] = getc(f);
	    }
	    for ( j=0; j<48; ++j ) {
		for ( k=j-1; k>=0; --k )
		    if ( stringoffsets[j]==stringoffsets[k] )
		break;
		if ( k!=-1 || stringoffsets[j]==0 )
	    continue;		/* this style doesn't exist */
		format = stringoffsets[j]-1;
		strlen = strings[0][0];
		if ( format!=0 && format!=-1 )
		    for ( k=0; k<strings[format][0]; ++k )
			strlen += strings[ strings[format][k+1]-1 ][0];
		pt = cur->psnames[j] = malloc(strlen+1);
		strcpy(pt,strings[ 0 ]+1);
		pt += strings[ 0 ][0];
		if ( format!=0 && format!=-1 )
		    for ( k=0; k<strings[format][0]; ++k ) {
			strcpy(pt,strings[ strings[format][k+1]-1 ]+1);
			pt += strings[ strings[format][k+1]-1 ][0];
		    }
		*pt = '\0';
	    }
	    cur->family = strdup(strings[0]);
	    for ( j=0; j<strcnt; ++j )
		free(strings[j]);
	    free(strings);
	}
	if ( doafm )
	    MakeAfmFiles(cur,f,isfixed,origfilename);
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
return( head );
}

static void SearchPostscriptResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, PSFONT *psfont) {
    long here = ftell(f);
    long *offsets, lenpos;
    int rname = -1, tmp;
    char name[300], newname[300];
    int ch1, ch2;
    static int ucnt;
    int len, type, i, rlen;
    /* I don't pretend to understand the rational behind the format of a */
    /*  postscript font. It appears to be split up into chunks where the */
    /*  maximum chunk size is 0x800, each section (ascii, binary, ascii, eof) */
    /*  has its own set of chunks (ie chunks don't cross sections) */
    char *buffer=NULL;
    int max = 0;
    FILE *pfb;

    fseek(f,rlistpos,SEEK_SET);
    offsets = calloc(subcnt,sizeof(long));
    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	tmp = (short) getushort(f);
	if ( rname==-1 ) rname = tmp;
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	offsets[i] = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
    }
    mytmpname(name);
    /* use the temp name until we've read the postscript font to see what it's name really is */
    if ( rname!=-1 ) {
	fseek(f,name_list+rname,SEEK_SET);
	ch1 = getc(f);
	fread(newname,1,ch1,f);
	strcpy(newname+ch1,".pfb");
    } else
	sprintf(newname,"Untitled-%d.pfb", ++ucnt );

    pfb = psfont!=NULL ? tmpfile() : fopen( name,"w" );
    if ( pfb==NULL ) {
	fprintf( stderr, "Can't open temporary file for postscript output\n" );
	fseek(f,here,SEEK_SET );
	free(offsets);
return;
    }

    putc(0x80,pfb);
    putc(1,pfb);
    lenpos = ftell(pfb);
    putc(0,pfb);
    putc(0,pfb);
    putc(0,pfb);
    putc(0,pfb);
    len = 0; type = 1;
    for ( i=0; i<subcnt; ++i ) {
	fseek(f,offsets[i],SEEK_SET);
	rlen = getlong(f);
	ch1 = getc(f); ch2 = getc(f);
	rlen -= 2;	/* those two bytes don't count as real data */
	if ( ch1==type )
	    len += rlen;
	else {
	    long hold = ftell(pfb);
	    fseek(pfb,lenpos,SEEK_SET);
	    putc(len&0xff,pfb);
	    putc((len>>8)&0xff,pfb);
	    putc((len>>16)&0xff,pfb);
	    putc(len>>24,pfb);
	    fseek(pfb,hold,SEEK_SET);
	    if ( ch1==5 )	/* end of font mark */
    break;
	    putc(0x80,pfb);
	    putc(ch1,pfb);
	    lenpos = ftell(pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    putc(0,pfb);
	    type = ch1;
	    len = rlen;
	}
	if ( rlen>max ) {
	    free(buffer);
	    max = rlen;
	    if ( max<0x800 ) max = 0x800;
	    buffer=malloc(max);
	    if ( buffer==NULL ) {
		fprintf( stderr, "Out of memory\n" );
		exit( 1 );
	    }
	}
	fread(buffer,1,rlen,f);
	if ( type==1 ) {
	    int j;
	    char *pt;
	    for ( j=0; j<rlen; ++j ) if ( buffer[j]=='\r' ) buffer[j] = '\n';
	    if ( i==0 && rname==-1 && (pt=strstr(buffer,"/FontName"))!=NULL ) {
		pt += strlen("/FontName");
		while ( isspace(*pt) ) ++pt;
		if ( *pt=='/' ) {
		    char *end = strchr(pt,' ');
		    ch1 = *end; *end='\0';
		    sprintf(newname,"%s.pfb", pt+1 );
		    *end = ch1;
		}
	    }
	}
	fwrite(buffer,1,rlen,pfb);
    }
    free(buffer);
    free(offsets);
    putc(0x80,pfb);
    putc(3,pfb);
    fseek(pfb,lenpos,SEEK_SET);
    putc(len&0xff,pfb);
    putc((len>>8)&0xff,pfb);
    putc((len>>16)&0xff,pfb);
    putc(len>>24,pfb);
    if ( psfont!=NULL ) {
	rewind(pfb);
	ParsePfb(pfb,psfont);
    }
    fclose(pfb);
    if ( psfont==NULL ) {
	if ( cleanfilename(newname)) {
	    if ( rename(name,newname)==-1 ) {
		fprintf( stderr, "Could not create %s\n", newname);
		unlink(name);
	    }
	} else
	    unlink(name);
    }
    fseek(f,here,SEEK_SET);
}

static int ttfnamefixup(FILE *ttf,char *buffer) {
    int version, isotf=false;
    int i,num, nameoffset, stringoffset;
    int fullval, famval, fullstr, famstr, fulllen, famlen, val, tag;
    int plat, spec, lang, name, len, off, ch;
    char *pt;

    rewind(ttf);
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
	nameoffset = getlong(ttf);
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
    strcpy(pt,isotf?".otf":".ttf");
return( true );
}

static void ttfnameset(FILE *ttf,char *curname,char *patheticattempt) {
    char buffer[1024];

    if ( !ttfnamefixup(ttf,buffer))
	strcpy(buffer,patheticattempt);
    if ( !cleanfilename(buffer))
	unlink(curname);
    else if ( rename(curname, buffer)==-1 ) {
	fprintf( stderr, "Could not create %s\n", buffer );
	unlink(curname);
    }
}

static void SearchTtfResources(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list, FOND *fonds) {
    long here, start = ftell(f);
    long roff;
    int rname = -1;
    char name[300], newname[300];
    int ch1, ch2;
    static int ucnt;
    int len, i, rlen, ilen;
    /* I think (hope) the sfnt resource is just a copy of the ttf file */
    char *buffer=NULL;
    int max = 0;
    FILE *ttf;

    /* PU -- Where TTFs are actually written */
    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	/* resource id = */ getushort(f);
	rname = (short) getushort(f);
	/* flags = */ getc(f);
	ch1 = getc(f); ch2 = getc(f);
	roff = rdata_pos+((ch1<<16)|(ch2<<8)|getc(f));
	/* mbz = */ getlong(f);
	here = ftell(f);
	if ( rname!=-1 ) {
	    fseek(f,name_list+rname,SEEK_SET);
	    ch1 = getc(f);
	    fread(newname,1,ch1,f);
	    strcpy(newname+ch1,".ttf");
	} else
	    sprintf(newname,"Untitled-%d.ttf", ++ucnt );

	mytmpname(name);
	ttf = fopen( name,"w+" );
	if ( ttf==NULL ) {
	    fprintf( stderr, "Can't open temporary file for truetype output.\n" );
	    fseek(f,here,SEEK_SET );
    continue;
	}

	fseek(f,roff,SEEK_SET);
	ilen = rlen = getlong(f);
	if ( rlen>16*1024 )
	    ilen = 16*1024;
	if ( ilen>max ) {
	    free(buffer);
	    max = ilen;
	    if ( max<0x800 ) max = 0x800;
	    buffer=malloc(max);
	}
	for ( len=0; len<rlen; ) {
	    int temp = ilen;
	    if ( rlen-len<ilen ) temp = rlen-len;
	    temp = fread(buffer,1,temp,f);
	    if ( temp==EOF )
	break;
	    fwrite(buffer,1,temp,ttf);
	    len += temp;
	}
	ttfnameset(ttf,name,newname);
	fclose(ttf);
	fseek(f,here,SEEK_SET);
    }
    fseek(f,start,SEEK_SET);
}

/* Look for a bare truetype font in a binhex/macbinary wrapper */
static int MightBeTrueType(FILE *binary,int pos,int dlen,char *filename) {
    FILE *out = fopen(filename,"w");
    char *buffer = malloc(8192);
    int len;

    if ( out==NULL ) {
	fprintf( stderr, "Could not open %s for writing\n", filename );
exit( 1 );
    }

    fseek(binary,pos,SEEK_SET);
    while ( dlen>0 ) {
	len = dlen > 8192 ? 8192 : dlen;
	len = fread(buffer,1,dlen > 8192 ? 8192 : dlen,binary);
	if ( len==0 )
    break;
	fwrite(buffer,1,len,out);
	dlen -= len;
    }
    fclose(out);
    free(buffer);
return( true );
}

static int IsResourceFork(FILE *f, long offset,char *filename, PSFONT *psfont) {
    /* If it is a good resource fork then the first 16 bytes are repeated */
    /*  at the location specified in bytes 4-7 */
    /* We include an offset because if we are looking at a mac binary file */
    /*  the resource fork will actually start somewhere in the middle of the */
    /*  file, not at the beginning */
    unsigned char buffer[16], buffer2[16];
    long rdata_pos, map_pos, type_list, name_list, rpos;
    long rdata_len, map_len;
    unsigned long tag;
    int i, cnt, subcnt;
    FOND *fondlist=NULL, *fl;

    fseek(f,offset,SEEK_SET);
    if ( fread(buffer,1,16,f)!=16 )
return( false );
    rdata_pos = offset + ((buffer[0]<<24)|(buffer[1]<<16)|(buffer[2]<<8)|buffer[3]);
    map_pos = offset + ((buffer[4]<<24)|(buffer[5]<<16)|(buffer[6]<<8)|buffer[7]);
    rdata_len = ((buffer[8]<<24)|(buffer[9]<<16)|(buffer[10]<<8)|buffer[11]);
    map_len = ((buffer[12]<<24)|(buffer[13]<<16)|(buffer[14]<<8)|buffer[15]);
    if ( rdata_pos+rdata_len!=map_pos )
return( false );
    fseek(f,map_pos,SEEK_SET);
    buffer2[15] = buffer[15]+1;	/* make it be different */
    if ( fread(buffer2,1,16,f)!=16 )
return( false );

/* Apple's data fork resources appear to have a bunch of zeroes here instead */
/*  of a copy of the first 16 bytes */
    for ( i=0; i<16; ++i )
	if ( buffer2[i]!=0 )
    break;
    if ( i!=16 ) {
	for ( i=0; i<16; ++i )
	    if ( buffer[i]!=buffer2[i] )
return( false );
    }
    getlong(f);		/* skip the handle to the next resource map */
    getushort(f);	/* skip the file resource number */
    getushort(f);	/* skip the attributes */
    type_list = map_pos + getushort(f);
    name_list = map_pos + getushort(f);

    if ( psfont==NULL ) {
	fseek(f,type_list,SEEK_SET);
	cnt = getushort(f)+1;
	for ( i=0; i<cnt; ++i ) {
	    tag = getlong(f);
	    subcnt = getushort(f)+1;
	    rpos = type_list+getushort(f);
	    if ( tag==CHR('F','O','N','D'))
		fondlist = BuildFondList(f,rpos,subcnt,rdata_pos,name_list,filename);
	}

	if ( trackps ) {
	    for ( fl = fondlist; fl!=NULL; fl=fl->next )
		for ( i=0; i<48; ++i ) if ( fl->psnames[i]!=NULL )
		    ProcessNestedPS( fl->psnames[i],filename, NULL );
	}
    }

    fseek(f,type_list,SEEK_SET);
    cnt = getushort(f)+1;
    for ( i=0; i<cnt; ++i ) {
	tag = getlong(f);
 /* printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );*/
	subcnt = getushort(f)+1;
	rpos = type_list+getushort(f);
	if ( tag==CHR('P','O','S','T'))		/* No FOND */
	    SearchPostscriptResources(f,rpos,subcnt,rdata_pos,name_list,psfont);
	else if ( psfont!=NULL )
	    /* No Op */;
	else if ( tag==CHR('F','O','N','T'))
	    SearchNFNTResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('N','F','N','T'))
	    SearchNFNTResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('s','f','n','t'))
	    SearchTtfResources(f,rpos,subcnt,rdata_pos,name_list,fondlist);
	else if ( tag==CHR('F','O','N','D'))
	    /* already parsed */;
	else
	    /* Ignored */;
    }
return( true );
}

#ifndef OldMacintosh
	/* OS/X and linux with appropriate drivers */
static int HasResourceFork(char *filename,PSFONT *psfont) {
    char *respath = malloc(strlen(filename)+strlen("/rsrc")+1);
    FILE *temp;
    int ret=false;

    strcpy(respath,filename);
    strcat(respath,"/rsrc");
    temp = fopen(respath,"r");
    free(respath);
    if ( temp!=NULL ) {
	ret = IsResourceFork(temp,0,filename,psfont);
	fclose(temp);
    }
return( ret );
}
#else
	/* OS/9 and before, these calls depreciated in X.4 */
static int HasResourceFork(char *filename,PSFONT *psfont) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
    FSRef ref;
    FSSpec spec;
    short res;
    long cnt, ret;
    FILE *temp;
    char *buf;

    if ( FSPathMakeRef( (unsigned char *) filename,&ref,NULL)!=noErr )
return( 0 );
    if ( FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)!=noErr )
return( 0 );
    if ( FSpOpenRF(&spec,fsRdPerm,&res)!=noErr )
return( 0 );
    temp = tmpfile();
    buf = malloc(8192);
    while ( 1 ) {
	cnt = 8192;
	ret = FSRead(res,&cnt,buf);
	if ( cnt!=0 )
	    fwrite(buf,1,cnt,temp);
	if ( ret==eofErr )
    break;
	if ( ret!=noErr )
    break;
    }
    free(buf);
    FSClose(res);
    rewind(temp);
    ret = IsResourceFork(temp,0,filename,psfont);
    fclose(temp);
return( ret );
}
#endif

static int IsResourceInBinary(FILE *f,char *filename, PSFONT *psfont) {
    unsigned char header[128], first[8];
    unsigned long offset, dlen, rlen;

    if ( fread(header,1,128,f)!=128 )
return( false );
    if ( header[0]!=0 || header[74]!=0 || header[82]!=0 || header[1]<=0 ||
	    header[1]>33 || header[63]!=0 || header[2+header[1]]!=0 )
return( false );
    dlen = ((header[0x53]<<24)|(header[0x54]<<16)|(header[0x55]<<8)|header[0x56]);
    rlen = ((header[0x57]<<24)|(header[0x58]<<16)|(header[0x59]<<8)|header[0x5a]);
/* Look for a bare truetype font in a binhex/macbinary wrapper */
    if ( dlen!=0 && rlen<=dlen) {
	int pos = ftell(f);
	fread(first,1,4,f);
	first[5] = '\0';
	if ( strcmp((char *) first,"OTTO")==0 || strcmp((char *) first,"true")==0 ||
		strcmp((char *) first,"ttcf")==0 ||
		(first[0]==0 && first[1]==1 && first[2]==0 && first[3]==0)) {
	    int len = header[1];
	    header[2+len] = '\0';
return( MightBeTrueType(f,pos,dlen,(char *) header+2));
	}
    }
	/* 128 bytes for header, then the dlen is padded to a 128 byte boundary */
    offset = 128 + ((dlen+127)&~127);
return( IsResourceFork(f,offset,filename,psfont));
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

static int IsResourceInHex(FILE *f,char *filename, PSFONT *psfont) {
    /* convert file from 6bit to 8bit */
    /* interesting data is enclosed between two colons */
    FILE *binary = tmpfile();
    char *sixbit = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";
    int ch, val, cnt, i, dlen, rlen, ret;
    char header[20], *pt;
    char wrappedfilename[128];

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
    for ( i=0, pt=wrappedfilename; i<ch; ++i )
	*pt++ = getc(binary);
    *pt = '\0';
    if ( getc(binary)!='\0' ) {
	fclose(binary);
return( false );
    }
    fread(header,1,20,binary);
    dlen = (header[10]<<24)|(header[11]<<16)|(header[12]<<8)|header[13];
    rlen = (header[14]<<24)|(header[15]<<16)|(header[16]<<8)|header[17];
/* Look for a bare truetype font in a binhex/macbinary wrapper */
    if ( dlen!=0 && rlen<dlen ) {
	int pos = ftell(binary);
	fread(header,1,4,binary);
	header[5] = '\0';
	if ( strcmp((char *) header,"OTTO")==0 || strcmp((char *) header,"true")==0 ||
		strcmp((char *) header,"ttcf")==0 ||
		(header[0]==0 && header[1]==1 && header[2]==0 && header[3]==0)) {
	    ret = MightBeTrueType(binary,pos,dlen,wrappedfilename);
	    fclose(binary);
return( ret );
	}
    }
    if ( rlen==0 ) {
	fclose(binary);
return( false );
    }

    ret = IsResourceFork(binary,ftell(binary)+dlen+2, filename,psfont);

    fclose(binary);
return( ret );
}

static int IsResourceInFile(char *filename,PSFONT *psfont) {
    FILE *f;
    char *spt, *pt;
    int ret;

    f = fopen(filename,"r");
    if ( f==NULL )
return( false );
    spt = strrchr(filename,'/');
    if ( spt==NULL ) spt = filename;
    pt = strrchr(spt,'.');
    
    /* BIN checking */
    if ( pt!=NULL && (pt[1]=='b' || pt[1]=='B') && (pt[2]=='i' || pt[2]=='I') &&
	    (pt[3]=='n' || pt[3]=='N') && pt[4]=='\0' ) {
	if ( IsResourceInBinary(f,filename,psfont)) {
	    fclose(f);
return( true );
	}
        /* HQX checking */
    } else if ( pt!=NULL && (pt[1]=='h' || pt[1]=='H') && (pt[2]=='q' || pt[2]=='Q') &&
	    (pt[3]=='x' || pt[3]=='X') && pt[4]=='\0' ) {
	if ( IsResourceInHex(f,filename,psfont)) {
	    fclose(f);
return( true );
	}
    }

    ret = IsResourceFork(f,0,filename,psfont);
    fclose(f);
    if ( !ret )
	ret = HasResourceFork(filename,psfont);
return( ret );
}

static int FindResourceFile(char *filename) {
    char *spt, *pt, *dpt;
    char buffer[1400];

    if ( IsResourceInFile(filename,NULL))
return( true );

    /* Well, look in the resource fork directory (if it exists), the resource */
    /*  fork is placed there in a seperate file on non-Mac disks */
    
    strncpy(buffer,filename, sizeof(buffer));
    spt = strrchr(buffer,'/');
    if ( spt==NULL ) { spt = buffer; pt = filename; }
    else { ++spt; pt = filename + (spt-buffer); }
    strcpy(spt,"resource.frk/");
    strcat(spt,pt);
    if ( IsResourceInFile(buffer,NULL))
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
	strncpy(dpt,exten,8);
    }
return( IsResourceInFile(buffer,NULL));
}

int fondu_main_simple(char *file) {
    force = true;
    return FindResourceFile(file);
}

int fondu_main(int argc, char **argv) {
    int i, ret = 0;

    for ( i=1; i<argc; ++i )
	if ( *argv[i]=='-' ) {
	    char *pt = argv[i];
	    if ( pt[1]=='-' ) ++pt;
	    if (strcmp(pt,"-latin1")==0 )
		tolatin1 = true;
	    else if (strcmp(pt,"-force")==0 || strcmp(pt,"-f")==0 )
		force = true;
	    else if (strcmp(pt,"-inquire")==0 || strcmp(pt,"-i")==0 )
		inquire = true;
	    else if (strcmp(pt,"-afm")==0 || strcmp(pt,"-a")==0 )
		doafm = true;
	    else if (strcmp(pt,"-trackps")==0 || strcmp(pt,"-t")==0 )
		trackps = true;
	    else if (strcmp(pt,"-show")==0 || strcmp(pt,"-s")==0 )
		show = true;
	    else {
		fprintf( stderr, "Usage: %s [-force] [-inquire] [-latin1] [-afm] [-trackps] [-show] macfiles\n",
		    argv[0] );
		fprintf( stderr, " if -force is given you will not be asked about replacing existing files\n" );
		fprintf( stderr, " if -inquire is given you will be asked about writing each file (whether\n" );
		fprintf( stderr, "\tit exists or not). -inquire overrides -force.\n" );
		fprintf( stderr, " if -show is given you will be told about each file created.\n" );
		fprintf( stderr, " if -latin1 is given nfnts will be reencoded to latin1 (else left in mac roman)\n" );
		fprintf( stderr, " if -trackps is given then any postscript files referenced by a FOND\n" );
		fprintf( stderr, "\twill be loaded and processed.\n" );
		fprintf( stderr, " if -afm is given then an incomplete afm file will be generated from any fonds.\n" );
		fprintf( stderr, " macfile may be a macbinary (.bin), binhex (.hqx) or bare resource fork.\n" );
		exit(0);
	    }
	} else if ( !FindResourceFile(argv[i])) {
	    fprintf( stderr, "Can't find an appropriate resource fork in %s\n", argv[i]);
	    ret=1;
	}
return( ret );
}
