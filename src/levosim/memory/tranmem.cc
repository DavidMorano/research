/* memdata */

/* Levo Transient Memory for Data */


#define	F_DEBUGS	0
#define	F_DEBUG		1


/* revision history :

	= 00/05/26, Dave Morano
	Borrowed and modifed from the IW object as a starting example.


	= 00/05/26, Alireza Khalafi
	I am going to be living here (and in obejct below) for some time !


*/


/******************************************************************************

	This object serves as the data memory subsystem for an initial
	Levo machine.

	This object is only concerned with data memory operations for
	the time being.


******************************************************************************/



#include	<sys/types.h>
#include	<cstdlib>
#include 	<cstdio>
#include	<cstring>

/*#include	<vsystem.h>*/

/*#include	"localmisc.h"*/
/* #include	"paramfile.h"*/
/* #include	"config.h"*/
/* #include	"defs.h"*/
/* #include	"mintinfo.h"*/
/* #include	"levoinfo.h"*/
/* #include	"busmd.h"*/		/* memory data bus to IW */

#include	"tranmem.h"		/* ourselves */
#include	"assert.h"

int print_tm(struct tranmem *tm, FILE * outfile);

void * Malloc(size_t s) {
	void * p;
	p = malloc(s);
	if (p == NULL) {
		printf("Malloc failed\n");
		exit(1);
	}
	return p;
}

int
main() {

int i,j;
FILE * outfile;
struct tranmem Mytm;
struct trminfo trmi;
struct mem_req mreq;

trmi.nblocks=32;   
trmi.nsubblocks=32;
trmi.nsets=2*256;    
trmi.ncols=4;   
trmi.assoc=1;  
trmi.stbufsize=10; 
trmi.ldbufsize=10;
tranmem_init(&Mytm, &trmi);

outfile=fopen("out1.lst", "wt");
assert(outfile);


mreq.addr = 10000;
mreq.value = 345;
mreq.row_tt = 4;
mreq.col_tt = 3;
mreq.path_id = 2;

for(j=0;j<trmi.nsubblocks;++j) {
for(i=1;i<256;++i) {
	mreq.row_tt = j;
	mreq.addr = i;
	mreq.value = i*2;
	write_tm(&Mytm, &mreq);
}
}
print_tm(&Mytm, outfile);
fclose(outfile);
printf("done ...\n");
return 0;
}

int
/*tranmem_init(tm,gp,pfp,mip,lip,tip)*/
tranmem_init(tm,tip) 
struct tranmem *tm;
/*struct global   *gp ;
PARAMFILE       *pfp ;
struct mintinfo *mip ;
struct levoinfo *lip ;*/
struct trminfo  *tip;
{
	int i,j;
	int num_of_entries;
	struct load_buff * lbp;
	struct store_buff * sbp;
	struct trm_subblock * subbp;
	struct trm_line * linep;
	struct ttreg * ttrp;


/*	tm -> gp = gp;
	tm -> mip = mip;
	tm -> lip = lip; */
/* allocate load and store buffers */
	tm -> lb = (struct load_buff *) Malloc (sizeof(struct load_buff) * tip -> nblocks);
	tm -> sb = (struct store_buff *) Malloc (sizeof(struct store_buff) * tip -> nblocks);
	lbp = tm -> lb;
	sbp = tm -> sb;

/* initialize load and store buffers */
	for(i=0; i < tip -> nblocks; ++i) 
	{
		lbp -> entry = (struct mem_req*) Malloc(sizeof(struct mem_req)* tip->ldbufsize);	
		lbp -> head = 0;
		lbp -> tail = 0;
		lbp -> buff_size = tip -> ldbufsize;
		++ lbp ;
	}
	
	for(i=0; i < tip -> nblocks; ++i) 
	{
		sbp -> entry = (struct mem_req*) Malloc(sizeof(struct mem_req)*tip->stbufsize);	
		sbp -> head = 0;
		sbp -> tail = 0;
		sbp -> buff_size = tip->stbufsize;
		++ sbp;
	}

/* allocate memory for the transient memory entries */

	tm -> bp = (struct trm_block *) Malloc(sizeof(struct trm_block));
	tm -> bp -> subb = (struct trm_subblock *) Malloc(sizeof(struct trm_subblock)*tip->nsubblocks);
	tm -> bp -> blocksize = tip -> nsubblocks;
	subbp = tm -> bp -> subb;
/* allocate sub block entries */
	for(i=0; i < tip->nsubblocks; ++i) 
	{
		subbp -> line = (struct trm_line *) Malloc(sizeof(struct trm_line)*tip->nsets);
		subbp -> row_timetag = i; /* reserve this subblock for time tag i */
		subbp -> subblocksize = tip -> nsets;
		linep = subbp -> line;
		for(j=0; j<tip -> nsets; ++j)
		{
	/*		linep -> value = (long *) Malloc (sizeof(long) * tip->ncols);*/
			linep -> valid = FALSE;
			linep -> linesize = tip -> ncols;
			linep ++;
		}	
		subbp ++;
	}


/* allocate memory for time-tag registers */

	num_of_entries = tip->ncols * tip->nsubblocks;
	tm -> ttregs = (struct ttreg *) Malloc (sizeof(struct ttreg)*tip->nsets);
	ttrp = tm -> ttregs;
	tm -> ttreg_size = tip -> nsets;
	for(i=0; i < tip->nsets; ++i)
	{
		ttrp -> entry = (char *)Malloc(sizeof(char)*num_of_entries);
		ttrp -> valid = FALSE;
		ttrp ++;
	}

} /* end subroutine: tranmem_init() */

int
find_tt(tm,mreq,ttp)
struct tranmem * tm;
struct mem_req * mreq;
int * ttp; 		/* time tag to read data from */
{
int tt = mreq -> tt; 
struct ttreg * ttrp = tm -> ttregs;
int j;

for(j=0; j < tm->ttreg_size; ++j) {
	/* search a time tag register to see if it has the same data tag */
}

}

int
find_subb(tm, rtt, subb)
struct tranmem * tm;
int * rtt; 		/* time tag to read data from */
struct trm_subblock *subb;
{


}

int
find_value(subb, mreq, rtt, val)
struct trm_subblock *subb;
struct memreq * mreq;
int * rtt; 		/* time tag to read data from */
long * val;
{


}



int
read_tm(tm, mreq)
struct tranmem * tm;
struct mem_req * mreq;
{
	int rtt;	/* time tag of temporary memory location to be read */
	int ret;	/* return value */
	struct trm_subblock subb;
	long val;
	

	ret = find_tt(tm, mreq, &rtt);
	if(ret == TM_MISS)
		return (TM_MISS);
	else if(ret == TM_CVALUE) 
	{
		val = tm -> ttregs -> cvalue;	
		return TM_CVALUE;
	}
	else if(ret == TM_HIT) 
	{
		find_subb(tm, rtt, &subb);
		find_value(subb, mreq, rtt, &val);
		mreq -> value = val;
		return (TM_HIT);
	}
	else
		assert(0);
}

int
find_line(subbp, addr, linepp)
struct trm_subblock * subbp;
int addr;
struct trm_line ** linepp; 
{
	int i,index;
	struct trm_line * linep;


	index = addr & (subbp -> subblocksize - 1); /* assume one way */
	linep = subbp -> line + index;
	if(linep -> valid == FALSE)
	{
		*linepp = linep;
		return TM_EMPTY;
	}
	else if(linep -> valid == TRUE) 
	{
		if(linep -> tag != addr)
		{	
			*linepp = NULL;
			return TM_CONF;
		} 
		else
		{
			*linepp = linep;
			return TM_HIT;	
		}
	}
}

int
write_tm(tm, mreq)
struct tranmem * tm;
struct mem_req * mreq;
{
	int ret;
	struct trm_subblock * subbp;
	struct trm_line * linep;

	subbp = tm -> bp -> subb + mreq -> row_tt;
	assert(subbp -> row_timetag == mreq -> row_tt);
	ret = find_line(subbp, mreq -> addr, &linep); 
	if(ret == TM_EMPTY) {
		linep -> tag = mreq -> addr;
		linep -> valid = TRUE;
		assert(linep -> linesize > mreq -> col_tt);
		linep -> value[mreq -> col_tt] = mreq -> value; 
	}
	else if (ret == TM_CONF) {
		/* store it in the store queue, add associativity */
		printf("conflict in address, %ld", mreq -> addr);
	}
	else if (ret == TM_HIT) {
		linep -> value[mreq -> col_tt] = mreq -> value;	
	}
}


int
print_tm(tm, outfile)
FILE * outfile;
struct tranmem * tm;
{

	int i,j,k;
	struct trm_subblock * subbp;
	struct trm_line * linep;

	subbp = tm -> bp -> subb;
	for(i=0; i< tm->bp->blocksize; ++i) {
		linep = subbp -> line;
		for(j=0; j<subbp->subblocksize;++j) {
			for(k=0; k < linep -> linesize; ++k) {
				fprintf(outfile,"R:%d,L:%d,C:%d a=%ld,v=%ld\n",i,j,k,linep -> tag, linep->value[k]);
			}
			linep++;
		}
		subbp++;
	}
}







/* here we have private object subroutines (interface shouldn't be public) */




