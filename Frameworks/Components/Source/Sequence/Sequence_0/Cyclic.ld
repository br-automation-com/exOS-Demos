PROGRAM _CYCLIC
<?xml version="1.0"?>
<?AutomationStudio FileVersion="4.9"?>
<!-- Automation Studio Generated XML Section -->
<networks maxcolumn="0">
	<network label="" comment="Monitor the deployment and execution of the remote program" row="10" column="0" commented="0">
		<row>
			<empty/>
			<empty/>
			<functionblock position="header" name="ExComponentInfo_0" type="ExComponentInfo"/>
			<empty/>
		</row>
		<row>
			<empty/>
			<contact type="address" name="gTarget_0"/>
			<functionblock position="top" type="ExComponentInfo"><input name="ExTargetLink" vardynamic="true"/><output name="Active"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<contact type="address" name="gSequence_0"/>
			<functionblock position="middle" type="ExComponentInfo"><input name="ExComponentLink" vardynamic="true"/><output name="Error"/></functionblock>
			<empty/>
		</row>
		<row>
			<contact type="open" name="TRUE"/>
			<line/>
			<functionblock position="middle" type="ExComponentInfo"><input name="Enable"/><output name="StatusID"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="ExComponentInfo"><output name="Name"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="ExComponentInfo"><output name="Deploying"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="ExComponentInfo"><output name="Operational"/></functionblock>
			<coil type="open" name="RemoteRun"/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="ExComponentInfo"><output name="Stopped"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="ExComponentInfo"><output name="Aborted"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="bottom" type="ExComponentInfo"><output name="Diag"/></functionblock>
			<empty/>
		</row>
	</network>
	<network label="" comment="Connect the Sequence_0 datamodel to remote system as soon as the remote program is running" row="7" column="0" commented="0">
		<row>
			<empty/>
			<empty/>
			<functionblock position="header" name="SequenceCyclic_0" type="SequenceCyclic"/>
			<empty/>
		</row>
		<row>
			<contact type="open" name="RemoteRun"/>
			<line/>
			<functionblock position="top" type="SequenceCyclic"><input name="Enable"/><output name="Active"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<analog type="input" name="SequenceInit_0.Handle"/>
			<functionblock position="middle" type="SequenceCyclic"><input name="Handle"/><output name="Error"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="SequenceCyclic"><input name="Start"/><output name="Disconnected"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<contact type="address" name="Sequence_0"/>
			<functionblock position="middle" type="SequenceCyclic"><input name="pSequence" vardynamic="true"/><output name="Connected"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="middle" type="SequenceCyclic"><output name="Operational"/></functionblock>
			<empty/>
		</row>
		<row>
			<empty/>
			<empty/>
			<functionblock position="bottom" type="SequenceCyclic"><output name="Aborted"/></functionblock>
			<empty/>
		</row>
	</network>
	<network label="" comment="Prevent the Sequence from automatically starting when not active (require release of buttons first)" row="2" column="0" commented="0">
		<row>
			<contact type="closed" name="Sequence_0.Active"/>
			<contact type="closed" name="ButtonLeft"/>
			<contact type="closed" name="ButtonRight"/>
			<coil type="open" name="StartOK"/>
		</row>
		<row>
			<empty or="true"/>
			<contact type="open" name="StartOK"/>
			<line or="true"/>
			<empty/>
		</row>
	</network>
	<network label="" comment="Only allow new start when not sequence is not active" row="2" column="0" commented="0">
		<row>
			<contact type="closed" name="Sequence_0.Active"/>
			<contact type="open" name="StartOK"/>
			<contact type="open" name="ButtonLeft"/>
			<coil type="open" name="Sequence_0.Buttons.ButtonLeft"/>
		</row>
		<row>
			<empty/>
			<empty or="true"/>
			<contact type="open" name="ButtonRight"/>
			<coil type="open" name="Sequence_0.Buttons.ButtonRight"/>
		</row>
	</network>
	<network label="" comment="Start (activate) the sequence if both buttons have been pressed simulataneously for 2 seconds" row="3" column="0" commented="0">
		<row>
			<empty/>
			<empty/>
			<functionblock position="header" name="TON_0" type="TON"/>
			<empty/>
		</row>
		<row>
			<contact type="open" name="Sequence_0.Buttons.ButtonLeft"/>
			<contact type="open" name="Sequence_0.Buttons.ButtonRight"/>
			<functionblock position="top" type="TON"><input name="IN"/><output name="Q"/></functionblock>
			<coil type="open" name="Sequence_0.Start"/>
		</row>
		<row>
			<empty/>
			<analog type="input" name="T#2s"/>
			<functionblock position="bottom" type="TON"><input name="PT"/><output name="ET"/></functionblock>
			<empty/>
		</row>
	</network>
</networks>
END_PROGRAM

