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

static void putshort(int val, FILE *f) {
    putc(val>>8,f);
    putc(val&0xff,f);
}

static void dousage(void) {
    fprintf( stderr, "Usage: setfondname -name=newname font.dfont\n" );
    exit(1);
}

static char *newname;

static void NewName(char *name) {
    if ( name[1]=='-' )
	++name;
    if ( strcmp(name,"-usage")==0 )
	dousage();
    else if ( strncmp(name,"-name=",6)==0 )
	newname = strdup(name+6);
    else
	dousage();
}

static void ClearNames(void) {
    newname = NULL;
}

/* There's probably only one fond in the file, but there could be more so be */
/*  prepared... */
/* http://developer.apple.com/techpubs/mac/Text/Text-269.html */
static void CheckFondList(FILE *f,long rlistpos,int subcnt,long rdata_pos,
	long name_list) {
    long start = ftell(f);
    int i;

    fseek(f,rlistpos,SEEK_SET);
    for ( i=0; i<subcnt; ++i ) {
	getushort(f);
	if ( feof(f)) {
	    fprintf(stderr, "EOF found in FOND list after reading %d resources of %d.\n", i, subcnt );
    break;
	}
	if ( newname!=NULL ) {
	    unsigned int namepos = ftell(f), nameloc;
	    fseek(f,0,SEEK_END);
	    nameloc = ftell(f);
	    putc(strlen(newname),f);
	    fputs(newname,f);
	    fseek(f,namepos,SEEK_SET);
	    putshort(nameloc-name_list,f);
	    newname = NULL;
	    fseek(f,namepos,SEEK_SET);
    break;
	}
	getushort(f);
	getc(f); getc(f); getc(f);
	getlong(f);
    }
    fseek(f,start,SEEK_SET);
return;
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
	    CheckFondList(f,rpos,subcnt,rdata_pos,name_list);
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
    temp = fopen(respath,"r+");
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

    f = fopen(filename,"r+");
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

    if ( argc==1 )
	dousage();

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' )
	    NewName(argv[i]);
	else if ( FindResourceFile(argv[i]))
	    ClearNames();
	else {
	    fprintf( stderr, "Can't find an appropriate resource fork in %s\n", argv[i]);
	    ret=1;
	}
    }
return( ret );
}
