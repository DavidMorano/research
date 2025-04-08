/* levo-specific */



#include	<sys/types.h>
#include	<string.h>

#include	"misc.h"
#include	"levo-specific.h"




int las_init(lasp,pid)
LAS	*lasp ;
int	pid ;
{


	memset(lasp,0,sizeof(LAS)) ;

	lasp->pid = pid ;
	return 0 ;
}


int las_load(lasp,idp)
LAS	*lasp ;
LID	*idp ;
{




}







