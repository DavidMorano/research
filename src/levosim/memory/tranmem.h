/* tranmem */

/* Levo Transient Memory for Data */


#ifndef	TRANMEM_INCLUDE
#define	TRANMEM_INCLUDE	1




#define	TRANMEM		struct tranmem



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"



#if	((! defined(TRUE)) && (! defined(FALSE))
#define FALSE	0
#define TRUE	1
#endif

#define TM_MISS -2
#define TM_CVALUE -1
#define TM_HIT 1
#define TM_EMPTY -3
#define TM_CONF -4

#define MAXNUMOFCOLS 36

struct trminfo {
	int  nblocks;	/* number of replicated blocks */
	int  nsubblocks;/* number of disjoint sub-blocks in each block */
	int  nsets;	/* number of sets in a sub-block */
	int  ncols;	/* number of columns (entries in a line) */
	int  assoc;	/* associativity for each sub-block */
	int  stbufsize; /* store buff size */
	int  ldbufsize; /* load buff size */
};

struct trm_line {
	long 	tag;		/* address tag */
	long	value[MAXNUMOFCOLS];		/* data values */
	char 	valid;
	char	dirty;
	int	linesize;
};

struct trm_subblock {
	struct trm_line  * line; /* cache lines */
	int	row_timetag;	 /* row time tag assigned to this sub-block */
	int	subblocksize;
};

struct trm_block {
	struct trm_subblock * subb;
	int	blocksize; 	/* number of sub-blocks in a block */
};
		
struct mem_req {
	long	addr;
	long	value;
	char 	width;		/*whether a word, half word or byte */
	int	tt;		/* time tag */
	int	row_tt;		/* row time tag */
	int	col_tt;		/* col time tag */
	int	path_id;	/* path id */
};

struct store_buff {
	struct mem_req	*entry;
	int		head;
	int		tail;
	int		buff_size;
};

struct load_buff {
	struct mem_req	*entry;
	int		head;
	int		tail;
	int		buff_size;
};

struct ttreg {
	char	* entry;	/* entries for the tt regsiter */	
	long	add;
	long	cvalue;		/* a copy of the value in next level of memory */
	int	valid;
	int	ttregsize;	/* number of entries in time tag register */
};

struct tranmem_state {
	int	stat0;
	int	stat1;
};

struct tranmem {
	struct global		*gp ;
	struct mintinfo		*mip;
	struct levoinfo		*lip;

	struct load_buff 	* lb;
	struct store_buff	* sb;
	
	struct trm_block	* bp;
	struct ttreg		* ttregs;
	int			ttreg_size;

	struct tranmem_state	c, n ;	/* synchronous clocked state */
} ;





#endif /* TRANMEM_INCLUDE */



