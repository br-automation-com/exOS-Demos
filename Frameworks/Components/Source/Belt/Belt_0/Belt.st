// This showcases a simplified version of a Belt with a programmed Actuator output
// In this case, all logic takes place in the ST application, meaning the function block BeltCyclic only exchanges the Belt_0 PUB/SUB structures between AR and Linux.
// Needless to say, more code should be added in order to achieve the correct behaviour, especially the actuator code should be moved inside the function block, that
// a burst of actuator set values can be buffered into a vector and is not ignored as it is now, where only the last Actuator On/Off value is used.

PROGRAM _INIT

END_PROGRAM

PROGRAM _CYCLIC
    
    //Autostart:
	BeltCyclic_0.Enable := ExComponentInfo_0.Operational;
	BeltCyclic_0.Start := BeltCyclic_0.Connected;
    
	IF (Belt_0.IO.iRun) THEN
		Belt_0.Status.Running := TRUE;
		Belt_0.Status.ActualSpeed := Belt_0.IO.iSpeed / 1000;
		Belt_0.Status.ActualPosition := (Belt_0.Status.ActualPosition + Belt_0.Status.ActualSpeed) MOD 1000;
		IF EDGEPOS(Belt_0.IO.iLatch) THEN
			Belt_0.Status.LatchedPosition := Belt_0.Status.ActualPosition;
			Belt_0.Status.Latched := TRUE;
		ELSE
			Belt_0.Status.Latched := FALSE;
		END_IF
	ELSE
		Belt_0.Status.Running := FALSE;
		Belt_0.Status.ActualSpeed := 0;
	END_IF
	
	//call the function block to update the Status structure
	BeltCyclic_0(pBelt := ADR(Belt_0));
    
	//The Actuator output is set according to the On/Off positions
	
	//take care of the 999 wrap around
	IF(lastPosition > Belt_0.Status.ActualPosition) THEN
		lastPosition := -1;
	END_IF
	
	IF (Belt_0.Status.LatchIndex <> Belt_0.Actuator.LatchIndex) THEN
		Belt_0.IO.oLatchActive := TRUE;
		IF (Belt_0.Status.ActualPosition >= Belt_0.Actuator.On) AND (lastPosition < Belt_0.Actuator.On) THEN
			Belt_0.Status.Actuator := TRUE;
		END_IF
		
		IF (Belt_0.Status.ActualPosition >= Belt_0.Actuator.Off) AND (lastPosition < Belt_0.Actuator.Off) THEN				
			Belt_0.Status.Actuator := FALSE;
			Belt_0.Status.LatchIndex := Belt_0.Actuator.LatchIndex;
		END_IF
	ELSE
		Belt_0.IO.oLatchActive := FALSE;
	END_IF
	
	lastPosition := Belt_0.Status.ActualPosition;
	
	Belt_0.IO.oActuator := Belt_0.Status.Actuator;
	Belt_0.IO.oActPos := DINT_TO_INT(Belt_0.Status.ActualPosition);
	
	ExComponentInfo_0(ExTargetLink := ADR(gTarget_0), ExComponentLink := ADR(gBelt_0), Enable := TRUE);
    
	ExDatamodelInfo_0(ExTargetLink := ADR(gTarget_0), Enable := TRUE, InstanceName := 'Belt_0');
    
END_PROGRAM

PROGRAM _EXIT

    BeltCyclic_0(Enable := FALSE);

END_PROGRAM
