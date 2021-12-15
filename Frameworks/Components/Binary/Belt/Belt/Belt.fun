FUNCTION_BLOCK BeltCyclic
	VAR_INPUT
		Enable : BOOL;
		Start : BOOL;
		pBelt : REFERENCE TO Belt;	END_VAR
	VAR_OUTPUT
		Connected : BOOL;
		Operational : BOOL;
		Error : BOOL;
	END_VAR
	VAR
		_Handle : UDINT;
		_Start : BOOL;
		_Enable : BOOL;
	END_VAR
END_FUNCTION_BLOCK

