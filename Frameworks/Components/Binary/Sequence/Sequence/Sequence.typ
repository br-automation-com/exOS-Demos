
TYPE
	SequenceButtons : 	STRUCT 
		ButtonLeft : BOOL;
		ButtonRight : BOOL;
	END_STRUCT;
	Sequence : 	STRUCT 
		Buttons : SequenceButtons; (*PUB*)
		Start : BOOL; (*PUB*)
		Active : BOOL; (*SUB*)
	END_STRUCT;
END_TYPE
