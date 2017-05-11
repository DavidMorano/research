/* main (pluser) */



#define	F_DEBUGS	0


#include	<sys/types.h>
#include	<string.h>
#include	<stdio.h>



#define	LINEBUFLEN	100



int main()
{
	int	rs, i, len ;

	char	linebuf[LINEBUFLEN + 1] ;


	while ((len = fgetline(stdin,linebuf,LINEBUFLEN)) > 0) {

#if	F_DEBUGS
		fprintf(stderr,"line len=%d\n",len) ;
#endif

		linebuf[len] = '\0' ;
		if (strncmp(linebuf,"+ ",2) == 0) {

			for (i = (len - 1) ; i > 0 ; i -= 1) {

				linebuf[i + 1] = linebuf[i] ;

			}

			linebuf[1] = '\t' ;
			linebuf[++len] = '\0' ;

		}

		fprintf(stdout,"%s",linebuf) ;

	} /* end while */

	fclose(stdout) ;

	fclose(stderr) ;

	return ;
}

