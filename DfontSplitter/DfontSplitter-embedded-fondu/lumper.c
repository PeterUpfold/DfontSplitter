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
/* This program takes two (or more) mac resource dfonts, merging any fonts */
/*  from the second (and subsequent) into the first. It handles resource id */
/*  conflicts */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned int	uint32;
typedef int		int32;
typedef unsigned short	uint16;
typedef short		int16;
typedef unsigned char	uint8;
typedef signed char	int8;

#define true	1
#define false	0

/* resource file format:
Start of file:
	long	Offset to start of resource data (always 0x100)
	long	Offset to start of resource map
	long	Length of resource data (map_offset-data_offset)
	long	Length of resource map
      pad with 0s to 0x100
Resource data
	for each resource
	 long	length of this resource
	 byte*n	resource data
	end
Resource map
	(repeat the initial 16 bytes for normal resource files, or
	 16 bytes of 0 for dfonts)
	long	0
	short	0
	short	0
	short	Offset from start of map to start of resource types (28?)
	short	Offset from start of map to start of resource names
Resource Types
	short	Number of different types-1
	for each type:
	 long	tag
	 short	number of resources of this type-1
	 short	offset to resource list
	end
Resource lists
	for each resource of the given type:
	 short	resource id
	 short	offset to name in resource name list (0xffff for none)
	 byte	flags
	 byte*3	offset from start of resource data section to this resource's data
	 long	0
	end
Resource names
	for each named resource
	 byte	name length
	 byte*n	name
	end
*/

struct resource_types {
    uint32		tag;
    struct resource	*res;
    struct resource_types *next;
    uint32 		pos, resloc;
    int			cnt;
};

struct resource {
    uint16		id, new_id;
    int			flags;
    FILE		*srcf;
    uint32		src_off;	/* Length is in the first long at this offset, data follow */
    char		*name;		/* Resource name */
    struct resource	*next;
    uint32		name_off;
    uint32		merged_off;
    uint32		nameptloc;
};

static struct resource_types *global_types;
static int do_fond_fixup = false;		/* only set if there's a resource id conflict */
static int only_fonts = true;			/* Copy every resource in first file, but only font resources in subsequent ones */


#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))
#define true 1
#define false 0

static int getushort(FILE *f) {
    int ch1 = getc(f);
    int ch2 = getc(f);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

static long getlong(FILE *f) {
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

static void putlong(long val, FILE *f) {
    putc((val>>24)&0xff,f);
    putc((val>>16)&0xff,f);
    putc((val>>8)&0xff,f);
    putc(val&0xff,f);
}


/* This routine is called after all resources have been written */
static void DumpResourceMap(FILE *res,struct resource_types *rtypes) {
    uint32 rfork_base = 0;
    uint32 resource_base = rfork_base+0x100;
    uint32 rend, rtypesstart, mend, namestart;
    struct resource_types *cur;
    struct resource *rcur;
    int i,j;

    fseek(res,0,SEEK_END);
    rend = ftell(res);

    for ( i=0; i<16; ++i )		/* 16 bytes of zeroes for a dfont */
	putc(0,res);

    putlong(0,res);			/* Some mac specific thing I don't understand */
    putshort(0,res);			/* another */
    putshort(0,res);			/* another */

    putshort(4+ftell(res)-rend,res);	/* Offset to resource types */
    putshort(0,res);			/* Don't know where the names go yet */

    rtypesstart = ftell(res);
    for ( i=0, cur=rtypes; cur!=NULL; cur = cur->next, ++i );
    putshort(i-1,res);			/* Count of different types */
    for ( cur=rtypes; cur!=NULL; cur = cur->next ) {
	putlong(cur->tag,res);		/* Resource type */
	putshort(0,res);		/* Number of resources of this type */
	putshort(0,res);		/* Offset to the resource list */
    }

    /* Now the resource lists... */
    for ( cur=rtypes; cur!=NULL; cur = cur->next ) {
	cur->pos = ftell(res);
	for ( rcur=cur->res; rcur!=NULL; rcur=rcur->next ) {
	    putshort(rcur->new_id,res);
	    rcur->nameptloc = ftell(res);
	    putshort(0xffff,res);		/* assume no name at first */
	    putc(rcur->flags,res);		/* resource flags */
		/* three byte resource offset */
	    putc( ((rcur->merged_off-resource_base)>>16)&0xff, res );
	    putc( ((rcur->merged_off-resource_base)>>8)&0xff, res );
	    putc( ((rcur->merged_off-resource_base)&0xff), res );
	    putlong(0,res);
	}
    }
    namestart = ftell(res);
    /* Now the names, if any */
    for ( cur=rtypes; cur!=NULL; cur = cur->next ) {
	for ( rcur=cur->res; rcur!=NULL; rcur=rcur->next ) {
	    if ( rcur->name!=NULL ) {
		rcur->name_off = ftell(res);
		putc(strlen(rcur->name),res);	/* Length */
		fwrite(rcur->name,1,strlen(rcur->name),res);
	    }
	}
    }
    mend = ftell(res);

    /* Repeat the rtypes list now we know where they go */
    fseek(res,rtypesstart+2,SEEK_SET);		/* skip over the count */
    for ( cur=rtypes; cur!=NULL; cur = cur->next ) {
	putlong(cur->tag,res);		/* Resource type */
	for ( j=0, rcur=cur->res; rcur!=NULL; rcur=rcur->next, ++j );
	putshort(j-1,res);		/* Number of resources of this type */
	putshort(cur->pos-rtypesstart,res);
    }
    /* And go back and fixup any name pointers */
    for ( cur=rtypes; cur!=NULL; cur = cur->next ) {
	for ( rcur=cur->res; rcur!=NULL; rcur=rcur->next ) {
	    if ( rcur->name!=NULL ) {
		fseek(res,rcur->nameptloc,SEEK_SET);
		putshort(rcur->name_off-namestart,res);
	    }
	}
    }

    fseek(res,rend,SEEK_SET);
    	/* Fixup duplicate header (and offset to the name list) */
    for ( i=0; i<16; ++i )
	putc(0,res);

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

static void DumpResource(FILE *to,struct resource *rcur) {
    uint32 len;
    int ch;

    rcur->merged_off = ftell(to);
    fseek(rcur->srcf,rcur->src_off,SEEK_SET);
    len = getlong(rcur->srcf);
    if ( len==EOF ) {
	fprintf( stderr, "Bad resource length at offset %d\n", rcur->src_off );
exit(1);
    }
    putlong(len,to);
    while ( len>0 ) {
	ch = getc(rcur->srcf);
	putc(ch,to);
	--len;
    }
}

static int FindNewId(uint32 tag,FILE *origf,int id) {
    struct resource_types *cur;
    struct resource *res;

    for ( cur= global_types; cur!=NULL; cur=cur->next ) {
	if ( cur->tag==tag ) {
	    for ( res=cur->res; res!=NULL; res=res->next ) {
		if ( res->id==id && res->srcf==origf )
return( res->new_id );
	    }
return( id );
	}
    }
return( id );
}

static void DumpFond(FILE *to,struct resource *rcur) {
    int i, assoc_cnt, size, style, id, newid;

    /* first dump the resource, then doctor it */
    DumpResource(to,rcur);
    
    fseek(to,rcur->merged_off+4+2*26,SEEK_SET);
    assoc_cnt = getushort(to)+1;
    for ( i=0; i<assoc_cnt; ++i ) {
	size = getushort(to);
	style = getushort(to);
	id = getushort(to);
	if ( size==0 )
	    newid = FindNewId(CHR('s','f','n','t'),rcur->srcf,id);
	else
	    newid = FindNewId(CHR('N','F','N','T'),rcur->srcf,id);
	if ( newid!=id ) {
	    fseek(to,-2,SEEK_CUR);
	    putshort(newid,to);
	    fseek(to,0,SEEK_CUR);	/* Have to do a seek to prepare for reading */
	}
    }
    fseek(to,0,SEEK_END);
}

static void ResCopy(FILE *to) {
    /* Copy everything from the global resource set to the result file */
    struct resource_types *cur;
    struct resource *rcur;
    int fixupfond;

    for ( cur=global_types; cur!=NULL; cur=cur->next ) {
	fixupfond = do_fond_fixup && cur->tag==CHR('F','O','N','D');
	for ( rcur = cur->res; rcur!=NULL; rcur=rcur->next )
	    if ( fixupfond )
		DumpFond(to,rcur);
	    else
		DumpResource(to,rcur);
    }
}

static int ResIdUsed(int id,struct resource *list1, struct resource *list2) {
    while ( list1!=NULL ) {
	if ( id == list1->new_id )
return( true );
	list1=list1->next;
    }
    while ( list2!=NULL ) {
	if ( id == list2->new_id )
return( true );
	list2=list2->next;
    }
return( false );
}

static void MergeResLists(struct resource_types *into, struct resource_types *from) {
    struct resource *res, *rtest, *rnext, *p;

    for ( res = from->res; res!=NULL; res = rnext ) {
	rnext = res->next;
	while ( ResIdUsed(res->new_id,into->res,rnext))
	    res->new_id = (res->new_id+1)&0xffff;
	if ( res->new_id!=res->id ) {
	    if ( into->tag==CHR('s','f','n','t') || into->tag==CHR('N','F','N','T'))
		do_fond_fixup = true;
	    fprintf( stderr, "Warning: Renumbering resource '%c%c%c%c' %d -> %d\n",
		    (into->tag>>24), ((into->tag>>16)&0xff), ((into->tag>>8)&0xff), (into->tag&0xff),
		    res->id, res->new_id );
	}
	for ( rtest=into->res, p=NULL; rtest!=NULL; p=rtest, rtest=rtest->next ) {
	    if ( rtest->new_id>res->new_id )
	break;
	}
	if ( p==NULL ) {
	    res->next = into->res;
	    into->res = res;
	} else {
	    res->next = p->next;
	    p->next = res;
	}
    }
    free(from);
}

static void MergeToGlobalResources(struct resource_types *head) {
    struct resource_types *test, *p, *hnext;

    if ( global_types==NULL ) {
	global_types = head;
return;
    }
    while ( head!=NULL ) {
	hnext = head->next;
	if ( only_fonts &&
		(head->tag!=CHR('s','f','n','t') && head->tag!=CHR('N','F','N','T') &&
		 head->tag!=CHR('F','O','N','D')) ) {
	    /* Copy everything in the first file, but only font resources in */
	    /*  subsequent ones */
	    head = hnext;
    continue;
	}
	for ( test=global_types; test!=NULL; test=test->next ) {
	    if ( test->tag==head->tag ) {
		MergeResLists(test,head);
	break;
	    }
	}
	if ( test==NULL ) {
	    /* I don't know if they are supposed to be ordered, but why not */
	    for ( test=global_types, p=NULL; test!=NULL; p=test, test=test->next ) {
		if ( test->tag>head->tag )
	    break;
	    }
	    if ( p==NULL ) {
		head->next = global_types;
		global_types = head;
	    } else {
		head->next = p->next;
		p->next = head;
	    }
	}
	head = hnext;
    }
}

static int IsResourceFork(FILE *f, long offset) {
    /* If it is a good resource fork then the first 16 bytes are repeated */
    /*  at the location specified in bytes 4-7 */
    /* Sigh. Unless it's a dfont */
    /* We include an offset because if we are looking at a mac binary file */
    /*  the resource fork will actually start somewhere in the middle of the */
    /*  file, not at the beginning */
    unsigned char buffer[16], buffer2[16];
    long rdata_pos, map_pos, type_list, name_list;
    int i, cnt;
    int ch1, ch2;
    int namepos;
    struct resource_types *head=NULL, *last=NULL, *cur;

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
	cur = calloc(1,sizeof(struct resource_types));
	cur->tag = getlong(f);
	cur->cnt = getushort(f)+1;
	cur->pos = type_list+getushort(f);
	if ( last==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }

    for ( cur = head; cur!=NULL; cur=cur->next ) {
	struct resource *rlast=NULL, *rcur;
	fseek(f,cur->pos,SEEK_SET);
	for ( i=0; i<cur->cnt; ++i ) {
	    rcur = calloc(1,sizeof(struct resource));
	    rcur->new_id = rcur->id = getushort(f);
	    namepos = getushort(f);
	    if ( namepos==0xffff )
		rcur->name_off = 0;
	    else
		rcur->name_off = name_list + namepos;
	    rcur->flags = getc(f);
	    ch1 = getc(f); ch2=getc(f);
	    rcur->src_off = rdata_pos + ((ch1<<16)|(ch2<<8)|getc(f));
	    rcur->srcf = f;
	    /* must be zero */ getlong(f);
	    if ( rlast==NULL )
		cur->res = rcur;
	    else
		rlast->next = rcur;
	    rlast = rcur;
	}
    }

    for ( cur = head; cur!=NULL; cur=cur->next ) {
	struct resource *rcur;
	for ( rcur=cur->res; rcur!=NULL; rcur=rcur->next ) {
	    if ( rcur->name_off!=0 ) {
		fseek(f,rcur->name_off,SEEK_SET);
		ch1 = getc(f);
		if ( ch1!=EOF ) {
		    rcur->name = malloc(ch1+1);
		    fread(rcur->name,1,ch1,f);
		    rcur->name[ch1] = '\0';
		}
	    }
	}
    }

    MergeToGlobalResources(head);
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
    if ( temp!=NULL )
	ret = IsResourceFork(temp,0);
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
return( true );
	}
    } else if ( pt!=NULL && (pt[1]=='h' || pt[1]=='H') && (pt[2]=='q' || pt[2]=='Q') &&
	    (pt[3]=='x' || pt[3]=='X') && pt[4]=='\0' ) {
	if ( IsResourceInHex(f)) {
return( true );
	}
    }

    ret = IsResourceFork(f,0);
    if ( !ret ) {
	fclose(f);
	ret = HasResourceFork(filename);
    }
return( ret );
}

static int LoadResourceMap(char *filename) {
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

static void CopyFile( FILE *to, FILE *from ) {
    int ch;

    rewind(from);
    while ( (ch=getc(from))!=EOF )
	putc(ch,to);
}

static void DumpResourceMaps(char *filename) {
    FILE *restemp = tmpfile(), *real;
    int i;
    char *pt, *dot, *new;

    /* Initial resource header, we'll fix it up later, for now just 0x100 bytes*/
    for ( i=0; i<0x100; ++i )
	putc('\0',restemp);

    /* The dump out the resource data */
    ResCopy(restemp);

    /* Then the resource map (which fixes the initial header too) */
    DumpResourceMap(restemp,global_types);

    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt=filename;
    dot = strrchr(pt,'.');
    if ( dot==NULL ) dot = pt+strlen(pt);
    new = malloc(strlen(pt)+strlen(".dfont")+2);
    strcpy(new,pt);
    strcpy(new+(dot-pt),".dfont");

    real = fopen(new,"w");
    if ( real==NULL ) {
	fprintf( stderr, "Can't open %s for output\n", new);
exit(1);
    }
    CopyFile(real,restemp);
    fclose(restemp);
    fclose(real);
}

int main(int argc, char **argv) {
    int i;

    if ( argc<3 ) {
	fprintf(stderr, "Usage: %s base.dfont additional.dfont ...\n\t merges any additional resource files into base.\n",
	    argv[0]);
	exit(1);
    }
    for ( i=1; i<argc; ++i )
	if ( !LoadResourceMap(argv[i])) {
	    fprintf( stderr, "Can't find an appropriate resource fork in %s\n", argv[i]);
	    exit(1);
	}
    DumpResourceMaps(argv[1]);
return( 0 );
}
