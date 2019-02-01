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

/* the resource fork may be opened (on Mac OS/X) by <filename>/rsrc */


/* tobin [-res rfilename] filename {[-res filename] rfilename} */
/* tobin filename -res rfilename */
/* tobin -res rfilename */

static void Usage(char *prog) {
    fprintf( stderr, "Usage: %s {[-res filename] [-create creat] [-type type] [filename]}\n" );
    fprintf( stderr, " -res filename\tProvides the name of a file whose data fork is to\n" );
    fprintf( stderr, "\t\t\tplaced in a resource fork\n" );
    fprintf( stderr, " -create creat\tProvides a four character creator (if omitted on the\n" );
    fprintf( stderr, "\t\t\tmac this will be read from the datafile.\n" );
    fprintf( stderr, " -type type\tProvides a four character type (if omitted on the\n" );
    fprintf( stderr, "\t\t\tmac this will be read from the datafile.\n" );
    fprintf( stderr, "Takes a list of filenames and perhaps a list of resource forks\n" );
    fprintf( stderr, "and puts the result into a list of macbinary files.\n" );
    fprintf( stderr, "On the mac, if no resource fork file is specified and the datafork\n" );
    fprintf( stderr, "file contains a resource fork, then it will be used as the resource\n" );
    fprintf( stderr, "fork.\n" );
    fprintf( stderr, "If no data filename is specified then the resource file will be\n" );
    fprintf( stderr, "placed in the macbinary file without a data fork.\n" );
    fprintf( stderr, "The output filename will be the name of the data fork file with .bin\n" );
    fprintf( stderr, "appended, or if there is no data fork file, then the resource fork file\n" );
    fprintf( stderr, "with .bin appended.\n" );
    exit( 1 );
}

static FILE *ResForkOfDataFile(char *dataname) {
#ifndef OldMacintosh
	/* OS/X and linux with appropriate drivers */
    char *respath = malloc(strlen(dataname)+strlen("/rsrc")+1);
    FILE *temp;

    strcpy(respath,dataname);
    strcat(respath,"/rsrc");
    temp = fopen(respath,"r");
    free(respath);
return( temp );
#elif __Mac		/* At 10.4 Mac starts warning that FSSpec is depreciated */
    /* copy the resource fork of dataname (if any) into the data fork of a temp file */
    FSRef ref;
    FSSpec spec;
    short res, err;
    long cnt;
    FILE *temp;
    unsigned char *buf;

    if ( dataname==NULL )
return( NULL );
    if ( FSPathMakeRef( (unsigned char *) dataname,&ref,NULL)!=noErr )
return( NULL );
    if ( FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)!=noErr )
return( NULL );
    if ( FSpOpenRF(&spec,fsRdPerm,&res)!=noErr )
return( NULL );
    temp = tmpfile();
    buf = malloc(8192);
    while ( 1 ) {
	cnt = 8192;
	err = FSRead(res,&cnt,buf);
	if ( cnt>0 )
	    fwrite(buf,1,cnt,temp);
	if ( err==eofErr )
    break;
	if ( err!=noErr )
    break;
    }
    free(buf);
    FSClose(res);
    rewind(temp);
return( temp );
#else
return( NULL );
#endif
}

static void FindTypeCreater(char *dataname,char **create,char **type) {
#if __Mac && !defined(OldMacintosh)
    static char cbuf[5], tbuf[5];
    FSRef ref;
    FSCatalogInfo info;

    if ( FSPathMakeRef( (unsigned char *) dataname,&ref,NULL)!=noErr )
return;
    if ( FSGetCatalogInfo(&ref,kFSCatInfoFinderInfo,&info,NULL,NULL,NULL)!=noErr )
return;
    if ( *type==NULL ) {
	tbuf[0] = ((FInfo *) (info.finderInfo))->fdType>>24;
	tbuf[1] = ((FInfo *) (info.finderInfo))->fdType>>16;
	tbuf[2] = ((FInfo *) (info.finderInfo))->fdType>>8;
	tbuf[3] = ((FInfo *) (info.finderInfo))->fdType;
	tbuf[4] = '\0';
	*type = tbuf;
    }
    if ( *create==NULL ) {
	cbuf[0] = ((FInfo *) (info.finderInfo))->fdCreator>>24;
	cbuf[1] = ((FInfo *) (info.finderInfo))->fdCreator>>16;
	cbuf[2] = ((FInfo *) (info.finderInfo))->fdCreator>>8;
	cbuf[3] = ((FInfo *) (info.finderInfo))->fdCreator;
	cbuf[4] = '\0';
	*create = cbuf;
    }
#elif defined(OldMacintosh)
    static char cbuf[5], tbuf[5];
    FSRef ref;
    FSSpec spec;
    FInfo info;

    if ( dataname==NULL )
return;
    if ( FSPathMakeRef( (unsigned char *) dataname,&ref,NULL)!=noErr )
return;
    if ( FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)!=noErr )
return;
    if ( FSpGetFInfo(&spec,&info)!=noErr )
return;
    if ( *type==NULL ) {
	tbuf[0] = info.fdType>>24;
	tbuf[1] = info.fdType>>16;
	tbuf[2] = info.fdType>>8;
	tbuf[3] = info.fdType;
	tbuf[4] = '\0';
	*type = tbuf;
    }
    if ( *create==NULL ) {
	cbuf[0] = info.fdCreator>>24;
	cbuf[1] = info.fdCreator>>16;
	cbuf[2] = info.fdCreator>>8;
	cbuf[3] = info.fdCreator;
	cbuf[4] = '\0';
	*create = cbuf;
    }
#else
    static char tbuf[5];
    int len = strlen(dataname);
    if ( *type==NULL ) {
	if (( len>4 && strcmp(dataname+len-4,".txt")==0 ) ||
		( len>4 && strcmp(dataname+len-4,".TXT")==0 ) ||
		( len>5 && strcmp(dataname+len-5,".text")==0 ) ||
		( len>5 && strcmp(dataname+len-5,".TEXT")==0 )) {
	    strcpy(tbuf,"TEXT");
	    *type = tbuf;
	}
    }
#endif
}

static void Dump(char *dataname, char *resname, char *create, char *type) {
    FILE *datafile=NULL, *resfile=NULL, *outfile;
    char *outname;
    unsigned char header[128], *hpt; char *pt, *ept;
    time_t now;
    unsigned long dlen, rlen;
    int i,crc, ch;

    if ( dataname==NULL && resname==NULL )
return;
    if ( dataname!=NULL ) {
	datafile = fopen(dataname,"r");
	if ( datafile==NULL ) {
	    fprintf(stderr,"Can't open %s for reading\n", dataname );
exit(1);
	}
	FindTypeCreater(dataname,&create,&type);
    }
    if ( resname!=NULL ) {
	resfile = fopen(resname,"r");
	if ( resfile==NULL ) {
	    fprintf(stderr,"Can't open %s for reading\n", resname );
exit(1);
	}
    } else
	resfile = ResForkOfDataFile(dataname);

    if ( dataname!=NULL ) {
	outname = malloc(strlen(dataname)+30);
	strcpy(outname,dataname);
	strcat(outname,".bin");
    } else {
	outname = malloc(strlen(resname)+30);
	strcpy(outname,resname);
	strcat(outname,".bin");
    }
    outfile = fopen(outname,"w");
    if ( outfile==NULL ) {
	fprintf( stderr, "Cannot open %s for writing\n", outname);
	free(outname);
	if ( datafile ) fclose(datafile);
	if ( resfile ) fclose(resfile);
    }

    memset(header,'\0',sizeof(header));
    hpt = header;
    *hpt++ = '\0';		/* version number */
    /* Mac Filename */
    pt = dataname?dataname:resname;
    ept = strrchr(pt,'/');
    if ( ept!=NULL ) pt = ept+1;
    ept = pt+strlen(pt);
    if ( ept-pt>63 ) ept=pt+63;
    *hpt++ = ept-pt;
    while ( pt<ept )
	*hpt++ = *pt++;
    while ( hpt<header+65 )
	*hpt++ = '\0';

    if ( type==NULL ) type = "\0\0\0\0";
    if ( create==NULL ) create = "\0\0\0\0";
    /* Mac File Type */
    *hpt++ = type[0]; *hpt++ = type[1]; *hpt++ = type[2]; *hpt++ = type[3];
    /* Mac Creator */
    *hpt++ = create[0]; *hpt++ = create[1]; *hpt++ = create[2]; *hpt++ = create[3];
    *hpt++ = '\0';		/* No finder flags set */
    *hpt++ = '\0';		/* (byte 74) MBZ */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Vert Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* Hor Position in folder */
    *hpt++ = '\0'; *hpt++ = '\0';	/* window or folder id??? */
    *hpt++ = '\0';		/* protected bit ??? */
    *hpt++ = '\0';		/* (byte 82) MBZ */

    dlen = rlen = 0;
    if ( datafile!=NULL ) {
	fseek(datafile,0,SEEK_END);
	dlen = ftell(datafile);
	fseek(datafile,0,SEEK_SET);
    }
    if ( resfile!=NULL ) {
	fseek(resfile,0,SEEK_END);
	rlen = ftell(resfile);
	fseek(resfile,0,SEEK_SET);
    }
	/* Data fork length */
    *hpt++ = dlen>>24; *hpt++ = dlen>>16; *hpt++ = dlen>>8; *hpt++ = dlen;
	/* Resource fork length */
    *hpt++ = rlen>>24; *hpt++ = rlen>>16; *hpt++ = rlen>>8; *hpt++ = rlen;

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

    fwrite(header,1,sizeof(header),outfile);
    if ( datafile ) {
	while ( (ch=getc(datafile))!=EOF )
	    putc(ch,outfile);
	while ( (dlen&127)!=0 )
	    { putc('\0',outfile); ++dlen; }
	fclose(datafile);
    }
    if ( resfile ) {
	while ( (ch=getc(resfile))!=EOF )
	    putc(ch,outfile);
	while ( (rlen&127)!=0 )
	    { putc('\0',outfile); ++rlen; }
	fclose(resfile);
    }
}

int main( int argc, char **argv) {
    int i;
    char *datafork=NULL, *resfork=NULL;
    char *create=NULL, *type=NULL;	/* these are sticky */

    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' ) {
	    char *pt = argv[i]+1;
	    if ( *pt=='-' ) ++pt;
	    if ( strcmp(pt,"usage")==0 || strcmp(pt,"help")==0 )
		Usage(argv[0]);
	    else if ( strcmp(pt,"version")==0 ) {
		printf( "tobin v1.0\n" );
		exit( 0 );
	    } else if ( strcmp(pt,"create")==0 || strcmp(pt,"creator")==0 || strcmp(pt,"creater")==0 || strcmp(pt,"c")==0 ) {
		if ( i+1>=argc ) {
		    fprintf( stderr, "%s must be followed by a filename\n", argv[i] );
		    exit( 1 );
		}
		if ( strlen(argv[i+1])!=4 ) {
		    fprintf( stderr, "the argument of %s must be four characters, but %s is not.\n",
			    argv[i], argv[i+1] );
exit( 0 );
		} else
		    create = argv[++i];
	    } else if ( strcmp(pt,"type")==0 || strcmp(pt,"t")==0 ) {
		if ( i+1>=argc ) {
		    fprintf( stderr, "%s must be followed by a filename\n", argv[i] );
		    exit( 1 );
		}
		if ( strlen(argv[i+1])!=4 ) {
		    fprintf( stderr, "the argument of %s must be four characters, but %s is not.\n",
			    argv[i], argv[i+1] );
exit( 0 );
		} else
		    type = argv[++i];
	    } else if ( strcmp(pt,"res")==0 || strcmp(pt,"rsrc")==0 || strcmp(pt,"resource")==0 || strcmp(pt,"r")==0 ) {
		if ( i+1>=argc ) {
		    fprintf( stderr, "%s must be followed by a filename\n", argv[i] );
		    exit( 1 );
		}
		if ( resfork!=NULL ) {
		    Dump(datafork,resfork,create,type);
		    datafork = NULL;
		    resfork = argv[++i];
		} else if ( datafork!=NULL ) {
		    Dump(datafork,argv[++i],create,type);
		    datafork = NULL;
		} else
		    resfork = argv[++i];
	    } else {
		fprintf( stderr, "Unrecognized argument %s\n", argv[i]);
		Usage(argv[0]);
	    }
	} else {
	    if ( datafork!=NULL ) {
		Dump(datafork,resfork,create,type);
		datafork = argv[i];
		resfork = NULL;
	    } else if ( resfork!=NULL ) {
		Dump(argv[i],resfork,create,type);
		resfork = NULL;
	    } else
		datafork = argv[i];
	}
    }
    if ( datafork!=NULL || resfork!=NULL )
	Dump(datafork,resfork,create,type);
return( 0 );
}
