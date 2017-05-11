/* instrclass */


/******************************************************************************

	These are some simple instruction class designations that are
	used in the execution window for making machine control
	changes.  These are determined by making an appropriate call to
	either the Levo Load Buffer (to query its instruction state) or
	to a Levo Active Station.


******************************************************************************/



#define	INSTRCLASS_UNUSED	0	/* unused, no instruction loaded */
#define	INSTRCLASS_NULL		1	/* null-instruction, predicate-only */
#define	INSTRCLASS_REG		2	/* regular ALU or nonspecial */
#define	INSTRCLASS_CBR		3	/* conditional branch or jump */
#define	INSTRCLASS_JMP		4	/* unconditional jump but not a call */
#define	INSTRCLASS_CALL		5	/* an unconditional call */
#define	INSTRCLASS_STORE	6	/* a store memory type instruction */




