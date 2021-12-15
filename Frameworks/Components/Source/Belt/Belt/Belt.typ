
TYPE
	BeltActuator : 	STRUCT 
		On : DINT;
		Off : DINT;
		LatchIndex : DINT;
	END_STRUCT;
	BeltStatus : 	STRUCT 
		Running : BOOL;
		Latched : BOOL;
		LatchIndex : DINT;
		Actuator : BOOL;
		ActualSpeed : DINT;
		ActualPosition : DINT;
		LatchedPosition : DINT;
	END_STRUCT;
	BeltIO : 	STRUCT 
		iRun : BOOL;
		iSpeed : INT;
		iLatch : BOOL;
		oActuator : BOOL;
		oActPos : INT;
		oLatchActive : BOOL;
	END_STRUCT;
	Belt : 	STRUCT 
		IO : BeltIO; (*I/O channels*)
		Status : BeltStatus; (*Current Belt Values PUB*)
		Actuator : BeltActuator; (*Actuator set values from Linux SUB*)
		Framework : BOOL; (*Linux Application is connected to Framework SUB*)
	END_STRUCT;
END_TYPE
