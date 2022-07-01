# wolfvoitool

## Overview

This project is a small utility to read the VoltageObjectInfo table of an AMD VBIOS, decode and dump several different types of entries within the table, as well as edit and/or append a new voltage object (creating/appending objects only works with mode INIT_REGULATOR.) Its intent is to allow one to send arbitrary data over I2C/SMBus with any bus on the GPU, to a device residing at any address - configuring it every time the GPU is initialized. The most common application of this is setting a fixed voltage offset (positive or negative) at the regulator, such that the offset will be applied to whatever voltage the GPU and/or its driver set.

## Background

AMD GPUs have a small flash chip on them which stores something called the VBIOS. This data influences both the actions of the driver, and that of internal parts of the GPU. The most commonly known function of the VBIOS (often used by cryptocurrency miners and hardcore overclockers) is storing memory timings, just as an example. Now... this VBIOS is split up into many different tables, and among them is one named VoltageObjectInfo. A voltage object has a type and mode - the type is what voltage rail it affects (for example, VDDC is GPU core voltage, MVDDC is memory voltage), and there's quite a few types of modes, but besides decoding some of them for display, there is only one we care about, which is INIT_REGULATOR. But before we can explore that, I should first expand a bit on regulators.

VRM controllers (regulators) handle the minute details of stepping down the +12V input voltage down to several lower voltages for different things (rails). They handle the limits for over-current/over-voltage/under-voltage/over-temperature faults, they decide what exact voltage is output for each rail, they can (sometimes) sense power draw on their output rails, and many, many more minor details of power delivery may be configured. This not only allows simple voltage control and power draw readouts, but allows access to very advanced overclocking parameters such as LLC (load-line calibration.) VRM controllers with software programmability often use SMBus (System Management Bus, loosely based on I2C.) The only details about SMBus that you need to know is that it functions over two wires, it has one master device which communicates with one or more slave devices, and finally, every SMBus slave on the bus will have an address.

Back to the voltage object - one with mode INIT_REGULATOR will have the information the GPU and its driver need to select an internal I2C bus and write whatever you please to a given I2C address on said bus. This, as the mode name implies, is done only once when the GPU is initialized. This provides a way to make the driver customize the configuration of an I2C/SMBus device (doesn't *actually* have to be a regulator/VRM controller, mind) according to your wishes every time that card is initialized.

## Example output

I did not commit the example ROM images to GitHub - if you want the source package with the example ROMs included, you may fetch it off my site for the current (as of this writing) version: https://lovehindpa.ws/code/releases/wolfvoitool/wolfvoitool-0.7.tar.xz


```
./wolfvoitool -f ExampleROMs/5700xtstock.rom 
wolfvoitool v0.70 by Wolf9466 (aka Wolf0/OhGodAPet)
Donation address (BTC): 1WoLFumNUvjCgaCyjFzvFrbGfDddYrKNR
VOI Table Format Revision 0x04, Content Revision 0x02.

VOI entry 0:
	VOType = 5	(Type "VDDGFX")
	VOMode = 3	(Mode "INIT_REGULATOR")
	Size = 54
	RegulatorID = 26
	I2CLine = 6
	I2CAddress = 96
	ControlOffset = 0
	Voltage entries are 8-bit.
	Data = 
		41007100370028004B0050005C004A00
		5D0014004D00880094005F004F000100
		5000E10093007D00FF00


VOI entry 1:
	VOType = 5	(Type "VDDGFX")
	VOMode = 7	(Mode "SVID2")
	Size = 12
	LoadLinePSI = 0x0006
		OffsetTrim = 2
		LoadLineSlopeTrim = 1
		PSI1: 0
		PSI0_EN: 0
		PSI0_VID: 0
	SVD GPIO ID: 0
	SVC GPIO ID: 0


VOI entry 2:
	VOType = 1	(Type "VDDC")
	VOMode = 3	(Mode "INIT_REGULATOR")
	Size = 54
	RegulatorID = 26
	I2CLine = 6
	I2CAddress = 96
	ControlOffset = 0
	Voltage entries are 8-bit.
	Data = 
		41007100370028004B0050005C004A00
		5D0014004D00880094005F004F000100
		5000E10093007D00FF00


VOI entry 3:
	VOType = 1	(Type "VDDC")
	VOMode = 7	(Mode "SVID2")
	Size = 12
	LoadLinePSI = 0x0004
		OffsetTrim = 0
		LoadLineSlopeTrim = 1
		PSI1: 0
		PSI0_EN: 0
		PSI0_VID: 0
	SVD GPIO ID: 0
	SVC GPIO ID: 2


VOI entry 4:
	VOType = 4	(Type "VDDCI")
	VOMode = 3	(Mode "INIT_REGULATOR")
	Size = 54
	RegulatorID = 26
	I2CLine = 6
	I2CAddress = 100
	ControlOffset = 0
	Voltage entries are 8-bit.
	Data = 
		4B0058005C0040002C0080004C005B00
		5D0014005E0084004D0088004F000100
		5000E10045003000FF00


VOI entry 5:
	VOType = 4	(Type "VDDCI")
	VOMode = 7	(Mode "SVID2")
	Size = 12
	LoadLinePSI = 0x000E
		OffsetTrim = 2
		LoadLineSlopeTrim = 3
		PSI1: 0
		PSI0_EN: 0
		PSI0_VID: 0
	SVD GPIO ID: 0
	SVC GPIO ID: 3


VOI entry 6:
	VOType = 2	(Type "MVDDC")
	VOMode = 3	(Mode "INIT_REGULATOR")
	Size = 54
	RegulatorID = 26
	I2CLine = 6
	I2CAddress = 100
	ControlOffset = 0
	Voltage entries are 8-bit.
	Data = 
		4B0058005C0040002C0080004C005B00
		5D0014005E0084004D0088004F000100
		5000E10045003000FF00


VOI entry 7:
	VOType = 2	(Type "MVDDC")
	VOMode = 7	(Mode "SVID2")
	Size = 12
	LoadLinePSI = 0x000E
		OffsetTrim = 2
		LoadLineSlopeTrim = 3
		PSI1: 0
		PSI0_EN: 0
		PSI0_VID: 0
	SVD GPIO ID: 0
	SVC GPIO ID: 1
```

## Warning

This code is a hastily written merge of two very old projects of mine, and as a result it has not been tested anywhere near enough. Use with caution.


## Donations accepted!

- BTC: `1WoLFumNUvjCgaCyjFzvFrbGfDddYrKNR`
- ETH: `0xCED1D126181874124A5Cb05563A4A8879D1CeC85`
