
FUNCTION_BLOCK RemoteDI
	VAR_INPUT
		Handle : UDINT;
		Name : STRING[80];
		Input : BOOL;
	END_VAR
	VAR_OUTPUT
		Ready : BOOL;
		Error : BOOL;
	END_VAR
	VAR
		_state : USINT;
		_input : BOOL;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK RemoteAI
	VAR_INPUT
		Handle : UDINT;
		Name : STRING[80];
		Input : INT;
	END_VAR
	VAR_OUTPUT
		Ready : BOOL;
		Error : BOOL;
	END_VAR
	VAR
		_state : USINT;
		_input : INT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK RemoteDO
	VAR_INPUT
		Handle : UDINT;
		Name : STRING[80];
	END_VAR
	VAR_OUTPUT
		Ready : BOOL;
		Error : BOOL;
		Output : BOOL;
	END_VAR
	VAR
		_state : USINT;
		_inst : UDINT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK RemoteIOInit
	VAR_OUTPUT
		Handle : UDINT;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK RemoteIOCyclic
	VAR_INPUT
		Enable : BOOL;
		Handle : UDINT;
		Start : BOOL;
	END_VAR
	VAR_OUTPUT
		Active : BOOL;
		Error : BOOL;
		Disconnected : BOOL;
		Connected : BOOL;
		Operational : BOOL;
		Aborted : BOOL;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK

FUNCTION_BLOCK RemoteIOExit
	VAR_INPUT
		Handle : UDINT;
	END_VAR
	VAR
		_state : USINT;
	END_VAR
END_FUNCTION_BLOCK
