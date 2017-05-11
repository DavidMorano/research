/* sgistat */


/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef SGISTAT_H
#define SGISTAT_H



#define SGI_ST_FSTYPSZ	16	/* array size for file system type name */
#define	SGI_ST_PAD1SZ	3	/* num longs in st_pad1 */
#define	SGI_ST_PAD2SZ	2	/* num longs in st_pad2 */
#define	SGI_ST_PAD4SZ	8	/* num longs in st_pad3 */

struct sgitimespec {
	int	tv_sec ;
	int	tv_nsec ;
} ;


/*
 * stat structure, used by stat(2) and fstat(2)
 */

struct sgistat {
	uint	st_dev;
	int	st_pad1[SGI_ST_PAD1SZ];	/* reserved for network id */
	uint	st_ino;
	uint	st_mode;
	uint	st_nlink;
	int	st_uid;
	int	st_gid;
	uint	st_rdev;
	int	st_pad2[SGI_ST_PAD2SZ];	/* dev and off_t expansion */
	int	st_size;
	int	st_pad3;	/* future off_t expansion */
	struct sgitimespec st_atim;	
	struct sgitimespec st_mtim;	
	struct sgitimespec st_ctim;	
	int	st_blksize;
	int	st_blocks;
	char	st_fstype[SGI_ST_FSTYPSZ];
	int	st_pad4[SGI_ST_PAD4SZ];	/* expansion area */
};



#endif	/* SGISTAT_H */
