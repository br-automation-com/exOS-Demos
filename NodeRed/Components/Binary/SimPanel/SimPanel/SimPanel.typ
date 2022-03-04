
TYPE
	SimPanelKnobs : 	STRUCT 
		P1 : INT;
		P2 : INT;
	END_STRUCT;
	SimPanelSwitches : 	STRUCT 
		DI1 : BOOL;
		DI2 : BOOL;
	END_STRUCT;
	SimPanelLED : 	STRUCT 
		Green : BOOL;
		Red : BOOL;
		Yellow : BOOL;
	END_STRUCT;
	SimPanelLEDs : 	STRUCT 
		DI1 : BOOL;
		DI2 : BOOL;
		DI3 : SimPanelLED;
		DI4 : SimPanelLED;
		DI5 : SimPanelLED;
		DI6 : SimPanelLED;
	END_STRUCT;
	SimPanelButtons : 	STRUCT 
		DI3 : BOOL;
		DI4 : BOOL;
		DI5 : BOOL;
		DI6 : BOOL;
		Encoder : BOOL;
	END_STRUCT;
	SimPanel : 	STRUCT 
		Switches : SimPanelSwitches; (*PUB*)
		Buttons : SimPanelButtons; (*PUB*)
		LEDs : SimPanelLEDs; (*SUB*)
		Knobs : SimPanelKnobs; (*PUB*)
		Display : INT; (*SUB*)
		Encoder : UINT; (*PUB*)
	END_STRUCT;
END_TYPE
