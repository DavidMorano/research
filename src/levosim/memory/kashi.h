/* kashi.h */

/* Levo Transient Memory for Data */


#ifndef	TRANMEM_INCLUDE
#define	TRANMEM_INCLUDE	1



#include	<sys/types.h>

#include	"misc.h"
#include	"config.h"
#include	"defs.h"
#include	"mintinfo.h"
#include	"levoinfo.h"



#define	TRANMEM		struct tranmem

#if	(! defined(FALSE)) && (! defined(TRUE))
#define FALSE	0
#define TRUE	1
#endif

#define TM_MISS -2
#define TM_CVALUE -1
#define TM_HIT 1
#define TM_EMPTY -3
#define TM_CONF -4

#define MAXNSETS	4
#define MAXNROWS	33
#define MAXNCOLS	9
#define MAXNPATHS	8
#define MAXNLDBUFFS	10
#define MAXNSTBUFFS	10
#define MAXNTTR		MAXNROWS*MAXNCOLS
#define MAXNPE		MAXNROWS*MAXNCOLS
#define MAXSTBUFSIZE	10
#define MAXLDBUFSIZE	10

#define TM_LDBUFSIZE	10
#define	TM_STBUFSIZE	10
#define TM_LOAD		1
#define	TM_STORE	2


struct trminfo {
	int  nrows;
	int  nsets;	/* number of lines in a block */
	int  npaths;
	int  ncols;	/* number of columns (entries in a line) */
	int  assoc;	/* associativity for each sub-block */
	int  stbufsize; /* store buff size */
	int  ldbufsize; /* load buff size */
};

struct trm_line {
	long 	addr;
	long	value;
	char 	valid;
	char	dirty;
};

struct mem_req {
	long	addr;
	long	value;
	char 	width;		/*whether a word, half word or byte */
	int	tt;		/* time tag */
	int	row_tt;		/* row time tag */
	int	col_tt;		/* col time tag */
	int	path_id;	/* path id */
	char	inst_type;	/* load or store */
};

struct store_buff {
	struct mem_req	entry[MAXSTBUFSIZE];
	int		head;
	int		tail;
	int		buff_size;
};

struct load_buff {
	struct mem_req	entry[MAXLDBUFSIZE];
	int		head;
	int		tail;
	int		buff_size;
};

struct ttreg {
	char	entry[MAXNPE];	/* entries for the tt regsiter */	
	long	addr_tag;
	long	cvalue;		/* a copy of the value in next level of memory */
	int	valid;
	int	ttreg_size;	/* number of entries in time tag register */
};

struct tt_unit {
	struct ttreg  reg[MAXNPATHS][MAXNTTR];
	int	nttregs;
	int 	ncols;
	int	nrows;
	int 	npaths;
	int	assoc;
};


struct tranmem_state {
	int	stat0;
	int	stat1;
};


struct trm_block {
	struct trm_line line[MAXNSETS];
	int	nsets;
	struct trm_line curr_line;

	int	row_tt;
	int	col_tt;
	int	path_id;

	char	inst_type;	/*whether load or store */
	int	next;		/* pointer to the next cell with the same address */
	int	prev;		/* pointer to the prev cell with the same address */
};


struct tranmem {
	struct global		*gp ;
	struct mintinfo		*mip;
	struct levoinfo		*lip;

	struct load_buff 	lb[MAXNLDBUFFS];
	int			nldbuffs;
	struct store_buff	sb[MAXNSTBUFFS];
	int			nstbuffs;
	
	struct trm_block	entry[MAXNROWS][MAXNCOLS][MAXNPATHS];
	int			nrows;
	int			ncols;
	int			npaths;
//	struct tt_unit		ttu;
	int			nttregs;

	struct tranmem_state	c, n ;	/* synchronous clocked state */
} ;

struct cache_entry {
	long	tag;
	char	valid;
	char	dirty;
};


struct oneway_cache {
	int	nsets;
	int	set_shift;
	int	blocksize;
	struct cache_entry * entry;
	long	miss_count;
	long	hit_count;
};



#endif /* TRANMEM_INCLUDE */



