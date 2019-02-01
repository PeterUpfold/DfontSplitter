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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#ifdef __Mac
#include <Files.h>

static int ToResourceFork(char *filename) {
    /* If we're on a mac, we can try to see if we've got a real resource fork */
#ifndef OldMacintosh
    char *buf, *pt, *respath;
    FILE *res, *temp;
    int cnt;

    respath = malloc(strlen(filename)+strlen("/rsrc")+1);
    strcpy(respath,filename);
    strcat(respath,"/rsrc");
    res = fopen(respath,"r");
    free(respath);

    buf = malloc(strlen(filename)+strlen(".res")+20);
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt=filename-1;
    strcpy(buf,pt+1);
    strcat(buf,".res");
    temp = fopen(buf,"w");
    if ( temp==NULL ) {
	fprintf( stderr, "Failed to open %s for output\n", buf);
return( -1 );
    }
    buf = malloc(8192);
    while ( 1 ) {
	cnt = 8192;
	cnt = fread(buf,1,cnt,temp);
	if ( cnt>0 )
	    fwrite(buf,1,cnt,temp);
	if ( cnt<=0 )
    break;
    }
    free(buf);
    fclose(res);
    fclose(temp);
return( 1 );
#else
    FSRef ref;
    FSSpec spec;
    short res;
    int cnt, ret;
    FILE *temp;
    char *buf, *pt;

    if ( FSPathMakeRef( (unsigned char *) filename,&ref,NULL)!=noErr )
return( 0 );
    if ( FSGetCatalogInfo(&ref,0,NULL,NULL,&spec,NULL)!=noErr )
return( 0 );
    if ( FSpOpenRF(&spec,fsRdPerm,&res)!=noErr )
return( 0 );
    buf = malloc(strlen(filename)+strlen(".res")+20);
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt=filename-1;
    strcpy(buf,pt+1);
    strcat(buf,".res");
    temp = fopen(buf,"w");
    if ( temp==NULL ) {
	fprintf( stderr, "Failed to open %s for output\n", buf);
return( -1 );
    }
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
    fclose(temp);
return( 1 );
#endif
}

static void Usage(char *prog) {
    fprintf( stderr, " Usage: %s files\n  Copies the resource forks of the specified files into data forks.\n", prog );
    exit( 1 );
}

int main(int argc, char **argv) {
    int i;

    if ( argc<=1 )
	    Usage(argv[0]);
    for ( i=1; i<argc; ++i ) {
	if ( *argv[i]=='-' )
	    Usage(argv[0]);
	else if ( ToResourceFork(argv[i])==-1 )
	    fprintf( stderr, "%s has no resource fork\n", argv[i]);
    }
return( 0 );
}
#endif

