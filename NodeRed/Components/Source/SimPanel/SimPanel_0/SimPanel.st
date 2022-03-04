
PROGRAM _INIT

    SimPanelInit_0();

END_PROGRAM

PROGRAM _CYCLIC
    
    //Auto connect:
    SimPanelCyclic_0.Enable := ExComponentInfo_0.Operational; // Component has been deployed and started up successfully
	SimPanelCyclic_0.Start := SimPanelCyclic_0.Connected;
    
	SimPanelCyclic_0(Handle := SimPanelInit_0.Handle, pSimPanel := ADR(SimPanel_0));
    
    ExComponentInfo_0(ExTargetLink := ADR(gTarget_0), ExComponentLink := ADR(gSimPanel_0), Enable := TRUE);
    
    ExDatamodelInfo_0(ExTargetLink := ADR(gTarget_0), Enable := TRUE, InstanceName := 'SimPanel_0');
    
END_PROGRAM

PROGRAM _EXIT

    SimPanelExit_0(Handle := SimPanelInit_0.Handle);

END_PROGRAM
