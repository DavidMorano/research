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

#include	"kashi.h"		/* ourselves */
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

trmi.nrows=32;   
trmi.nsets=4;    
trmi.ncols=8;   
trmi.npaths=4;
trmi.assoc=1;  
trmi.stbufsize=10; 
trmi.ldbufsize=10;
tranmem_init(&Mytm, &trmi);

outfile=fopen("out1.lst", "wt");
assert(outfile);


mreq.path_id = 2;

for(j=0;j<trmi.nrows;++j) {
	for(i=1;i<trmi.ncols;++i) {
	mreq.row_tt = j;
	mreq.col_tt = i;
	mreq.addr = i;
	mreq.value = i*2;
	write_tm(&Mytm, &mreq);
}
}

mreq.inst_type = TM_STORE;
mreq.row_tt = 13;
mreq.col_tt = 4;
mreq.addr = 2000;
mreq.value = 7777;
write_tm(&Mytm, &mreq);

mreq.row_tt = 11;
mreq.col_tt = 2;
mreq.addr = 2000;
mreq.value = 9999;
write_tm(&Mytm, &mreq);

mreq.inst_type = TM_LOAD;
mreq.row_tt = 10;
mreq.col_tt = 1;
mreq.addr = 2000;
mreq.value = 0;
read_tm(&Mytm, &mreq);
printf("Value at addr:%ld=%ld\n",mreq.addr, mreq.value);

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
	int i,j,k;


/*	tm -> gp = gp;
	tm -> mip = mip;
	tm -> lip = lip; */

	tm -> nldbuffs = tip -> ldbufsize;
	tm -> nstbuffs = tip -> stbufsize;
	tm -> nrows = tip -> nrows;
	tm -> ncols = tip -> ncols;
	tm -> npaths = tip -> npaths;
	tm -> nttregs = tip -> nrows * tip -> ncols * tip -> npaths * tip -> nsets;
	for(i=0; i<tm->nldbuffs; ++i)
	{
		tm -> lb[i].head = 0;
		tm -> lb[i].tail = 0;
		tm -> lb[i].buff_size = TM_LDBUFSIZE;
	}
	for(i=0; i<tm->nldbuffs; ++i)
	{
		tm -> sb[i].head = 0;
		tm -> sb[i].tail = 0;
		tm -> sb[i].buff_size = TM_STBUFSIZE;
	}
	for(i=0; i<tm->nrows; ++i)
		for(j=0; j<tm->ncols; ++j)
			for(k=0; k<tm->npaths; ++k) {
				tm -> entry[i][j][k].row_tt = i;
				tm -> entry[i][j][k].col_tt = j;
				tm -> entry[i][j][k].path_id = k;
				tm -> entry[i][j][k].nsets = 0;
				tm -> entry[i][j][k].curr_line.valid = FALSE;
				/* need to initialize the extra lines */
			}

/*
	tm -> ttu.nttregs = tm->nrows * tm -> ncols;
	tm -> ttu.nrows = tm -> nrows;
	tm -> ttu.ncols = tm -> ncols;
	tm -> ttu.npaths = tm -> npaths;
	for(j=0;j<tm->ttu.npaths; ++j)
		for(i=0; i<(tm->ttu.nttregs); ++i)
		{	
			tm -> ttu.reg[i][j].valid = FALSE;	
			tm -> ttu.reg[i][j].ttreg_size = tm -> nrows * tm -> ncols;
		}
*/

} /* end subroutine: tranmem_init() */


/*
int
udpate_ttu(ttu, mreq)
struct tt_unit * ttu;
struct mem_req * mreq;
{
	long index;
	int entryindex;
	struct ttreg * regp;

	index = mreq -> addr & (ttu -> nttregs - 1); // Direct mapped time tag buffer 
	regp = ttu -> reg[mreq -> path_id][index];
	entryindex = mreq -> row_tt + mreq -> col_tt << ttu -> ncols;
	if(regp -> valid == TRUE && mreq -> addr != regp -> addr_tag)
	{
		printf("conflict in Time tag unit \n");
		retrun TM_CNFL;
	} 


// reset the entry in other tt reg with the same time tag 
	for(i=0; i<ttu->nttregs; ++i)
		ttu->reg[mreq->path_id][i].entry[entryindex] = 0;

	if(regp->valid == FALSE) {
		assert(entryindex < regp -> ttreg_size);	
		regp -> entry[entryindex] = 1;
		regp -> valid = TRUE;
		regp -> addr_tag = mreq -> addr;
	} 
	else if(mreq -> addr == regp -> addr_tag) {
		assert(entryindex < regp -> ttreg_size);	
		regp -> entry[entryindex] = 1;
	}
	else
	
} */

int
write_tm(tm, mreq)
struct tranmem * tm;
struct mem_req * mreq;
{
	int ret;
	struct trm_line * linep;

	linep = &(tm -> entry[mreq->row_tt][mreq->col_tt][mreq->path_id].curr_line);
	
	if(linep->valid == FALSE) {
		linep -> value = mreq -> value;
		linep -> addr = mreq -> addr;
	}
	else {
		/* conflict */	
		linep -> value = mreq -> value;
		linep -> addr = mreq -> addr;
	}
/*	update_ttu(&tm->ttu, mreq, oldaddr); */
}

read_tm(tm, mreq)
struct tranmem * tm;
struct mem_req * mreq;
{
	int ret;
	int i,j;
	struct trm_line * linep;

	linep = &(tm -> entry[mreq->row_tt][mreq->col_tt][mreq->path_id].curr_line);
	
assert(mreq -> inst_type == TM_LOAD);

	for(j=mreq -> row_tt; j >= 0; --j) {
		linep = & tm -> entry[j][mreq->col_tt][mreq->path_id].curr_line;
		if(linep -> addr == mreq -> addr) {
			mreq -> value = linep -> value;	
			return TM_HIT;
		}
	}


	for(i=mreq -> col_tt; i >= 0; --i)
		for(j=tm -> nrows; j >= 0; --j)
		{
			linep = & tm -> entry[j][i][mreq->path_id].curr_line;
			if(linep -> addr == mreq -> addr) {
				mreq -> value = linep -> value;	
				return TM_HIT;
			}
		}


/* did not find an entry in the Transient Memory, look at higher levels of memory */
	return TM_MISS;
}


int
print_tm(tm, outfile)
FILE * outfile;
struct tranmem * tm;
{

	int i,j,k;
	long val, add;

	for(i=0; i<tm->nrows; ++i)
		for(j=0; j<tm->ncols; ++j)
			for(k=0; k<tm->npaths; ++k) {
				val = tm -> entry[i][j][k].curr_line.value;
				add = tm -> entry[i][j][k].curr_line.addr;
				fprintf(outfile,"R:%d,C:%d,P:%d a=%ld,v=%ld\n",i,j,k, add, val);
			}

}



int
init_oneway_cache(cp, log2nsets, blocksize)
struct oneway_cache * cp;
int log2nsets;
int blocksize;
{
	cp -> nsets = 1 << cp -> set_shift;
	cp -> set_shift = log2nsets;
	cp -> blocksize = blocksize;
	cp -> entry = (struct cache_entry *) Malloc(sizeof(struct cache_entry)*cp->nsets);
}		

int
read_oneway_cache(cp, addr, valuep)
struct oneway_cache * cp;
long addr;
long * valuep;
{
	long set_index;
	struct cache_entry * cep;
		
	set_index = addr & (cp->nsets-1);
	cep = cp -> entry + set_index;
	if(cep -> tag == addr && cep -> valid == TRUE) {
		// Cache Hit
		/* valuep = get_value_from mint */
		return TM_HIT;
	}
	else if (cep->valid == FALSE)
	{
		/* cold miss */
		return TM_MISS;
	}
	else  
	{
		/* Cache Conflict */
		return TM_CONF;
	}	
} 


int
write_oneway_cache(cp, addr, valuep)
struct oneway_cache * cp;
long addr;
long * valuep;
{
	long set_index;
	struct cache_entry * cep;
		
	set_index = addr & (cp->nsets-1);
	cep = cp -> entry + set_index;
	if(cep -> tag == addr && cep -> valid == TRUE) {
		// Cache Hit
		/* valuep = write value to mint */
		return TM_HIT;
	}
	else if (cep->valid == FALSE)
	{
		/* cold miss */
		return TM_MISS;
	}
	else  
	{
		/* Cache Conflict */
		return TM_CONF;
	}	
} 


int
update_oneway_cache(cp, addr)
struct oneway_cache * cp;
long addr;
{
	long set_index;
	struct cache_entry * cep;
		
	set_index = addr & (cp->nsets-1);
	cep = cp -> entry + set_index;
	cep -> tag = addr;
	cep -> valid = TRUE;
}


/* here we have private object subroutines (interface shouldn't be public) */




