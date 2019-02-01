/* Copyright (C) 2002-2003 by George Williams */
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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#if __Mac
# include <CoreServices.h>		/* -I/Developer/Headers/FlatCarbon/ */
#endif

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



/* frombin filenames */

static void Usage(char *prog) {
    fprintf( stderr, "Usage: %s [-usage] [-help] [-version] filenames\n" );
    fprintf( stderr, " -usage\t\tPrints this message\n" );
    fprintf( stderr, " -help\t\tPrints this message\n" );
    fprintf( stderr, " -version\t\tPrints the version of the program\n" );
    fprintf( stderr, "Takes a list of macbinary filenames and extracts the contents\n" );
    fprintf( stderr, " on the mac it creates the obvious file.\n" );
    fprintf( stderr, " on non-mac systems it will create up to three files for each input file\n");
    fprintf( stderr, "  *.info contains some mac specific data\n" );
    fprintf( stderr, "  *.data contains the data fork (if present)\n" );
    fprintf( stderr, "  *.rsrc contains the resource fork (if present)\n" );
    exit( 1 );
}

static void ProcessFile(char *filename) {
    FILE *binfile;
    unsigned char header[128];
    char name[80];
    int dlen, rlen;
    int i,ch;

    binfile = fopen(filename,"r");
    if ( binfile==NULL ) {
	fprintf( stderr, "Cannot open %s\n", filename);
return;
    }
    fread(header,1,sizeof(header),binfile);
    if ( header[0]!=0 || header[74]!=0 || header[82]!=0 || header[1]<=0 ||
	    header[1]>33 || header[63]!=0 || header[2+header[1]]!=0 ) {
	fprintf( stderr, "%s does not look like a macbinary file\n", filename );
	fclose(binfile);
return;
    }
    strncpy(name,(char *) header+2,header[1]);
    name[header[1]] = '\0';
    dlen = ((header[0x53]<<24)|(header[0x54]<<16)|(header[0x55]<<8)|header[0x56]);
    rlen = ((header[0x57]<<24)|(header[0x58]<<16)|(header[0x59]<<8)|header[0x5a]);
    fprintf( stderr, " %s => %s, dfork len=%d rfork len=%d\n", filename, name, dlen, rlen );
#ifndef __Mac
    {
	FILE *datafile, *resfile, *infofile;

	if ( dlen>0 ) {
	    fseek(binfile,128,SEEK_SET);
	    strcpy(name+header[1],".data");
	    datafile = fopen(name,"w");
	    if ( datafile==NULL )
		fprintf( stderr, "Cannot open output file: %s\n", name );
	    else {
		for ( i=0; i<dlen && (ch=getc(binfile))!=EOF; ++i )
		    putc(ch,datafile);
		fclose(datafile);
	    }
	}

	if ( rlen>0 ) {
	    fseek(binfile,128 + ((dlen+127)&~127),SEEK_SET);
	    strcpy(name+header[1],".rsrc");
	    resfile = fopen(name,"w");
	    if ( resfile==NULL )
		fprintf( stderr, "Cannot open output file: %s\n", name );
	    else {
		for ( i=0; i<rlen && (ch=getc(binfile))!=EOF; ++i )
		    putc(ch,resfile);
		fclose(resfile);
	    }
	}
	
	strcpy(name+header[1],".info");
	infofile = fopen(name,"w");
	if ( infofile==NULL ) {
	    fprintf( stderr, "Cannot open output file: %s\n", name );
	} else {
	    name[header[1]] = '\0';
	    fprintf( infofile, "Mac filename = %s\n", name );
	    fprintf( infofile, "Data fork length = %d\n", dlen );
	    fprintf( infofile, "Resource fork length = %d\n", rlen );
	    fprintf( infofile, "File Type = %c%c%c%c\n",
		    header[65], header[66], header[67], header[68]);
	    fprintf( infofile, "File Creator = %c%c%c%c\n",
		    header[69], header[70], header[71], header[72]);
	    fprintf( infofile, "Finder Flags = %08x\n",
		    header[73]);
	    fclose( infofile );
	}
    }
#elif !defined(OldMacintosh)
    {
	FILE *datafile, *resfile;
	FSRef ref;

	if ( dlen>0 || rlen>0 ) {
	    fseek(binfile,128,SEEK_SET);
	    name[header[1]]='\0';
	    datafile = fopen(name,"w");
	    if ( datafile==NULL )
		fprintf( stderr, "Cannot open output file: %s\n", name );
	    else {
		for ( i=0; i<dlen && (ch=getc(binfile))!=EOF; ++i )
		    putc(ch,datafile);
		fclose(datafile);

		if ( rlen>0 ) {
		    fseek(binfile,128 + ((dlen+127)&~127),SEEK_SET);
		    strcpy(name+header[1],"/rsrc");
		    resfile = fopen(name,"w");
		    if ( resfile==NULL )
			fprintf( stderr, "Cannot open output file: %s\n", name );
		    else {
			for ( i=0; i<rlen && (ch=getc(binfile))!=EOF; ++i )
			    putc(ch,resfile);
			fclose(resfile);
		    }
		}

		/* Set type/creator */
		if ( FSPathMakeRef( (unsigned char *) name,&ref,NULL)==noErr ) {
		    FSCatalogInfo info;
		    /* Finder info contains more than type/creator. So don't */
		    /*  change the other values */
		    if ( FSGetCatalogInfo(&ref,kFSCatInfoFinderInfo,&info,NULL,NULL,NULL)==noErr ) {
			((FInfo *) (info.finderInfo))->fdType =
				(header[65]<<24)|(header[66]<<16)|(header[67]<<8)|header[68];
			((FInfo *) (info.finderInfo))->fdCreator =
				(header[69]<<24)|(header[70]<<16)|(header[71]<<8)|header[72];
			FSSetCatalogInfo(&ref,kFSCatInfoFinderInfo,&info);
		    }
		}
	    }
	}
    }
#else		/* __Mac */
    {
	FILE *file;
	FSRef ref;
	FSSpec spec;
	int creator, type;
	long len;
	short macfile;
	unsigned char *buf;

	file = fopen( name,"w");
	if ( file==NULL )
	    fprintf(stderr, "Cannot open output file: %s\n", name );
	else {
	    /* First the data fork */
	    fseek(binfile,128,SEEK_SET);
	    for ( i=0; i<dlen && (ch=getc(binfile))!=EOF; ++i )
		putc(ch,file);
	    fclose(file);

	    /* Then the resource fork */
	    type = (header[65]<<24)|(header[66]<<16)|(header[67]<<8)|header[68];
	    creator = (header[69]<<24)|(header[70]<<16)|(header[71]<<8)|header[72];
	    if ( FSPathMakeRef( (unsigned char *) name,&ref,NULL)==noErr &&
		    FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)==noErr ) {
		FSpCreateResFile(&spec,creator,type,smSystemScript);
		if ( FSpOpenRF(&spec,fsWrPerm,&macfile)==noErr ) {
		    SetEOF(macfile,0);		/* Truncate it just in case it existed... */
		    fseek(binfile,128 + ((dlen+127)&~127),SEEK_SET);
		    buf = malloc(8*1024);
		    for ( i=0; i<rlen && (len=fread(buf,1,8*1024,binfile))>0 ; i+= len ) {
			if ( i+len>rlen )
			    len = rlen-i;
			FSWrite(macfile,&len,buf);
		    }
		    FSClose(macfile);
		    free(buf);
		}
	    }
	}
    }
#endif
    fclose(binfile);
}

int main( int argc, char **argv) {
    int i;

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    char *pt = argv[i]+1;
	    if ( *pt=='-' ) ++pt;
	    if ( strcmp(pt,"usage")==0 || strcmp(pt,"help")==0 )
		Usage(argv[0]);
	    else if ( strcmp(pt,"version")==0 ) {
		printf( "frombin v1.0\n" );
		exit( 0 );
	    } else {
		fprintf( stderr, "Unrecognized argument %s\n", argv[i]);
		Usage(argv[0]);
	    }
	} else {
	    ProcessFile(argv[i]);
	}
    }
return( 0 );
}
