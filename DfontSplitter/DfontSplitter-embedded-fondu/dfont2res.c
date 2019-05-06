/* Copyright (C) 2002 by George Williams */
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

static void Usage(char *prog) {
    fprintf(stderr, "Usage: %s mac-dfont {mac-dfonts}\n", prog );
    fprintf(stderr, "\tTakes a list of mac-dfont files and generates corresponding resource\n" );
    fprintf(stderr, "\tfork files (in a macbinary wrapper)\n" );
    exit(0);
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

static void CopyFile(FILE *out, FILE *in, char *filename) {
    struct macbinaryheader header;
    char buffer[1024], *pt;
    unsigned char ubuf[16];
    int len, map_pos;

    memset(buffer,0,128);
    fwrite(buffer,1,128,out);		/* Fake Mac binary header */

    /* Copy the file */
    while ( (len=fread(buffer,1,1024,in))>0 )
	fwrite(buffer,1,len,out);
    /* dfonts have a slightly different map header than do resource forks */
    /*  make the header appropriate for a fork */
    rewind(in);
    fread(ubuf,1,16,in);
    map_pos = 128 + ((ubuf[4]<<24)|(ubuf[5]<<16)|(ubuf[6]<<8)|ubuf[7]);
    fseek(out,map_pos,SEEK_SET);
    fwrite(buffer,1,16,out);

    /* And now the macbinary header */
    pt = strrchr(filename,'/');
    if ( pt==NULL )
	strcpy(buffer,filename);
    else
	strcpy(buffer,pt+1);
    pt = strstr(buffer,".dfont");
    if ( pt==NULL )
	pt = strrchr(buffer,'.');
    if ( pt!=NULL ) *pt = '\0';

    header.type = CHR('F','F','I','L');
    header.creator = CHR('D','M','O','V');
    header.macfilename = buffer;
    DumpMacBinaryHeader(out,&header);
}

int main( int argc, char **argv) {
    int i;
    FILE *in, *out;
    char buffer[1024], *pt;

    if ( argc==1 )
	Usage(argv[0]);
    for ( i=1; i<argc; ++i ) {
	if ( strcmp(argv[i],"-help")==0 || strcmp(argv[i],"-h")==0 )
	    Usage(argv[0]);
	strcpy(buffer,argv[i]);
	pt = strstr(buffer,".dfont");
	if ( pt==NULL ) pt = strrchr(buffer,'.');
	if ( pt!=NULL )
	    strcpy(pt,".bin");
	else
	    strcat(buffer,".bin");
	in = fopen(argv[i],"r");
	out = fopen(buffer,"w");
	if ( in==NULL || out==NULL ) {
	    fprintf(stderr, "Can't open input or output file for %s, %s\n",
		    argv[i], buffer);
exit(1);
	}
	CopyFile(out,in,argv[i]);
	fclose(out);
	fclose(in);
    }
return(0);
}
