//This is an example of serializing different datapoints avialable via user FUBs to a single datamodel.
//The RemoteIO datamodel lies internally in the RemoteIOCyclic, and each function block accesses the
//same (serialized) datasets if changes occur on the IOs. The datasets are identified via Name, and 
//the values are deserialized in the function blocks and in the Linux application.

//This allows for a flexible number of in- and outputs while using a compiled (binary) datamodel, 
//whereas it requires a bit more programming (than available in this example) to handle the 
//serialization / deserialization correctly.

PROGRAM _INIT

    RemoteIOInit_0();

END_PROGRAM

PROGRAM _CYCLIC
    
    //Auto connect:
    RemoteIOCyclic_0.Enable := ExComponentInfo_0.Operational;
    RemoteIOCyclic_0(Handle := RemoteIOInit_0.Handle);
    
	RemoteDI_Switch(Handle := RemoteIOInit_0.Handle, Name := 'Switch D1');
	RemoteDO_Switch(Handle := RemoteIOInit_0.Handle, Name := 'Switch D1');

	RemoteDI_Button(Handle := RemoteIOInit_0.Handle, Name := 'Button DI3');
	RemoteDO_Button(Handle := RemoteIOInit_0.Handle, Name := 'Button DI3');

	IF (Extra) THEN
		RemoteDI_Extra(Handle := RemoteIOInit_0.Handle, Name := 'Extra D4');
		RemoteDO_Extra(Handle := RemoteIOInit_0.Handle, Name := 'Extra D4');
	END_IF
	
	RemoteAI_Pot(Handle := RemoteIOInit_0.Handle, Name := 'Potentiometer P1');

	IF (RemoteDI_Switch.Ready AND RemoteDO_Switch.Ready AND RemoteDI_Button.Ready AND RemoteDO_Button.Ready AND RemoteAI_Pot.Ready) THEN
		RemoteIOCyclic_0.Start := TRUE;
	END_IF
	
    ExComponentInfo_0(ExTargetLink := ADR(gTarget_0), ExComponentLink := ADR(gRemoteIO_0), Enable := TRUE);
    
    ExDatamodelInfo_0(ExTargetLink := ADR(gTarget_0), Enable := TRUE, InstanceName := 'RemoteIO_0');
    
END_PROGRAM

PROGRAM _EXIT

    RemoteIOExit_0(Handle := RemoteIOInit_0.Handle);

END_PROGRAM
