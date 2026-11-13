/* netorder HEADER */
/* charset=ISO8859-1 */
/* lang=C++20 (conformance reviewed) */

/* convert integer type variables to and from "net" order */
/* version %I% last-modified %G% */


/* Copyright © 1998 David A­D­ Morano.  All rights reserved. */

#ifndef	NETORDER_INCLUDE
#define	NETORDER_INCLUDE


#include	<envstandards.h>	/* MUST be first to configure */
#include	<sys/types.h>
#include	<limits.h>
#include	<localmisc.h>


/* object define */
#define	NETORDER	char *


EXTERNC_begin
extern int netorder_rchar	(void *,char *) noex ;
extern int netorder_rshort	(void *,short *) noex ;
extern int netorder_rint	(void *,int *) noex ;
extern int netorder_rlong	(void *,long *) noex ;
extern int netorder_rll		(void *,long *) noex ;
EXTERNC_end

#if	(long_BIT == 64)
#define	netorder_rlong64(a,b)	netorder_rlong((a),(b))
#else
#define	netorder_rlong64(a,b)	netorder_rll((a),(b))
#endif

EXTERNC_begin
extern int netorder_ruchar	(void *,uchar *) noex ;
extern int netorder_rushort	(void *,ushort *) noex ;
extern int netorder_ruint	(void *,uint *) noex ;
extern int netorder_rulong	(void *,ulong *) noex ;
extern int netorder_rull	(void *,ulong *) noex ;
EXTERNC_end

#if	(long_BIT == 64)
#define	netorder_rulong64(a,b)	netorder_rulong((a),(b))
#else
#define	netorder_rulong64(a,b)	netorder_rull((a),(b))
#endif

EXTERNC_begin
extern int netorder_wchar	(void *,int) noex ;
extern int netorder_wshort	(void *,int) noex ;
extern int netorder_wint	(void *,int) noex ;
extern int netorder_wlong	(void *,long) noex ;
extern int netorder_wll		(void *,long) noex ;
EXTERNC_end

#if	(long_BIT == 64)
#define	netorder_wlong64(a,b)	netorder_wlong((a),(b))
#else
#define	netorder_wlong64(a,b)	netorder_wll((a),(b))
#endif

#define	netorder_wuchar(A,B)	netorder_wchar((A),(B)) ;
#define	netorder_wushort(A,B)	netorder_wshort((A),(B)) ;
#define	netorder_wuint(A,B)	netorder_wint((A),(B)) ;
#define	netorder_wulong(A,B)	netorder_wlong((A),(B)) ;
#define	netorder_wull(A,B)	netorder_wll((A),(B)) ;

#if	(long_BIT == 64)
#define	netorder_wulong64(a,b)	netorder_wulong((a),(b))
#else
#define	netorder_wulong64(a,b)	netorder_wull((a),(b))
#endif


/* older API */


#define	netorder_reads(A,B)	netorder_rshort((A),(B)) ;
#define	netorder_readi(A,B)	netorder_rint((A),(B)) ;
#define	netorder_readl(A,B)	netorder_rlong((A),(B)) ;

#define	netorder_writes(A,B)	netorder_wshort((A),(B)) ;
#define	netorder_writei(A,B)	netorder_wint((A),(B)) ;
#define	netorder_writel(A,B)	netorder_wlong((A),(B)) ;


#endif /* NETORDER_INCLUDE */


