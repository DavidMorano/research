/* check for a valid OPEXEC value */
int lexeccheck(opcode)
int	opcode ;
{
	int	rs ;


	if ((opcode < 0) || (opcode >= LEXECOP_MAX))
		rs = LEXEC_RUNIMPL ;

	else
		rs = LEXEC_ROK ;

	return rs ;
}
/* end subroutine (lexeccheck) */



