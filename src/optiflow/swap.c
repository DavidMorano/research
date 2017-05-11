/* swap */

/* support subroutines for swapping bytes in various length words */


/* Copyright © 2003-2007 David A­D­ Morano.  All rights reserved. */


#include	"host.h"
#include	"endian.h"


/* unsigned */

half_t swap_half(half_t v)
{
	return SWAP_HALF(v) ;
}


word_t swap_word(word_t v)
{
	return SWAP_WORD(v) ;
}


qword_t swap_qword(qword_t v)
{
	return SWAP_QWORD(v) ;
}


/* signed */

shalf_t swap_shalf(shalf_t v)
{
	return SWAP_HALF((half_t) v) ;
}


sword_t swap_sword(sword_t v)
{
	return SWAP_WORD((word_t) v) ;
}


sqword_t swap_sqword(sqword_t v)
{
	return SWAP_QWORD((qword_t) v) ;
}


