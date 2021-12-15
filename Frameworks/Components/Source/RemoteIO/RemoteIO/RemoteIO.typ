
TYPE
	RemoteIOAnalog : 	STRUCT 
		Name : STRING[80];
		State : INT;
	END_STRUCT;
	RemoteIODigital : 	STRUCT 
		Name : STRING[80];
		State : BOOL;
	END_STRUCT;
	RemoteIO : 	STRUCT 
		AnalogInput : RemoteIOAnalog; (*PUB*)
		DigitalInput : RemoteIODigital; (*PUB*)
		DigitalOutput : RemoteIODigital; (*SUB*)
		NewAnalogInput : STRING[80]; (*PUB*)
		NewDigitalInput : STRING[80]; (*PUB*)
		NewDigitalOutput : STRING[80]; (*PUB*)
	END_STRUCT;
END_TYPE
