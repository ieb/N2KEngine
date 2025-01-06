# N2K Engine Monitor

This monitors an engine, mostly temperatures and fuel, emitting suitable N2K messages. Targeting a Volvo Penta D2-40. Code and pcb are based on the interface board in https://github.com/ieb/EngineManagement, however this time the pcb is from scratch using a Atmel 328p directly to make the unit more robust. The original has been in service for 3 years with no problems seen, outlasting the Volvo Penta MDI unit, which failed 2x in < 4 years. Ok, the replacement was based on relays following the Perkins schematic for the same engine, so not really hard to do.

# PCB

Single board. If using a 328p chip you will need to flash the bootloader. See programBootLoader for details. 

<div>
<img alt="board" src="screenshots/Screenshot 2024-12-17 at 16.58.52.png" />
</div>

Ran out of tinning solution so this board was manually tinned, hence the soldering looks a bit of a mess in places, but the copper is protected.

## Changes from original

* All components SMD, no draughtboards or modules with pin headers to fail.
* Switched from a LM385 to a LMV393 for the flywheel pulse generator as all I need is a clean square wave to count the pulses. (actually I miss ordered 10 LMV393's and then switched over).
* Changed to a 16MHz crystal on the MCP2515 since this is what the 328p is using. Perhaps 1 crystal would work for both, but probably not.



# Memory usage

after reducing memory requirements (580 used before)

    RAM:   [==        ]  23.8% (used 487 bytes from 2048 bytes)
    Flash: [======    ]  63.1% (used 20366 bytes from 32256 bytes)

after 1 wire addition and better cli

    RAM:   [===       ]  28.8% (used 589 bytes from 2048 bytes)
    Flash: [========  ]  81.2% (used 26198 bytes from 32256 bytes)    

NB, no malloc in use in code.

# Status bits.

The status bits in PGN 127489 are set to indicate an alarm condition.

WHen an alarm condition is set if the check engine, maintenance or emergency stop bits are set, they are not
cleared when the alarm clears. Generally advisable to stop the engine and check.

Emergency stop will be set when temperatures are too high. Best to stop the engine before damage occurs.

Over Temperature status 1 bit 1 set when over temperature is suspected including coolant (> 90C), exhaust > 80C, alternator > 100C, engine room > 70C. Will also set check engine and engine emergency shutdown alarms.

Low oil pressure alarm is set when oil pressure is < 10psi.

If sensors fail then engine comms alarm is set.

Water flow alarm is set when th exhaust temperature > 80C as there is probably no raw water.

Charge indicator alarm is set when the alternator voltage is < 12.8V

Engine shutdown alarm will be set when the engine is shutting down.

Other alarms are not set at this time.

# Building and Uploading

## Attiny3226 based board

Use UDPI. I use the old, slow and slightly buggy jtag2udpi programmer loaded onto an Uno. I use https://github.com/ieb/jtag2updi forked just in case. After programming the Uno needs a 10uF cap between reset and ground to prevent a reset on the Uno when programming starts.  The UDPI output is on Digital Pin 6 and connects via a 4K7 Resistor to the UDPI pin on the Attiny3226. The target board should be powered separately, and the serial monitor can remain attached while programming. Do not power the board using both a serial converter and the 12v power supply as this will cause the serial port to draw too much USB current and be shutdown. Programing setup is in platformio.ini. Before programming a fresh chip, set the fuses to set the CPU Frequency.

Obviously edit platform.ini with the correct serial ports for upload and monitoring.

    pio run -e attiny3226 -t fuses

Then upload code with an upload command. Fuses only need to be set once per chip.

    pio run -e attiny3226 -t upload

To monitor 

    pio device monitor

### Clock Tuning 3226

The 3226 board uses an internal oscilator due to lack of pins and so it needs tuning from a board with a quartz crystal as per https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/Ref_Tuning.md. The Tuning source needs to be https://github.com/SpenceKonde/megaTinyCore/tree/master/megaavr/libraries/megaTinyCore/examples/TuningSource uploaded onto a Uno or any 328p with pin 9 available. The UNO must have an real crystal not a resonator.  The tuning code is https://github.com/SpenceKonde/megaTinyCore/tree/master/megaavr/libraries/megaTinyCore/examples/megaTinyTuner, but for these boards must be modified to use PB1 as the timing pin since this is exposed directly. (Oil pressure sense).  Copies of both of these are in tuning as they needed minor modifications.


## 328p based board

Can be built on any 328p suitably wired, however to build using the pcb design first flash a bootloader, see [programBootLoader/README.md] for details, then come back here.

Having flashed with the urbootloader connect a serial adapter configured for 5V to the serial header pins, adjust platform.ini to reflect the serial port and run

    pio run -t upload

to monitor

    pio device monitor

Do not attempt to power the board both on the 12v line and through the serial port 5v at the same time. This will probably shutdown the USB port on your laptop (or worse produce some smoke) Monitoring over serial can be done provided only GND, TX and RX are attached. Ie no power from the serial adapter.

# Todo

* [x] build board.
* [x] Flash bootloader and test
* [x] Port code from original
* [x] Allow for 16MHz MCP2515 oscillator, current library uses 8MHz.
* [x] Handle when engine not running, now that device is powered from CanBus
* [x] Reduce memory requirements in EngineSensorts and bind measurements to messages.
* [x] PR SmallNMEA2000 with changes
* [x] Test and Calibrate NTCs
* [x] Test and Calibrate Coolant and fuel
* [x] Test On N2K Bus
* [x] Implement 1 wire
* [x] Allow Engine Hours to be set via serial
* [x] Improve status and diagnostics to include n2k traffic
* [x] Implement status alarms
* [x] Consider swapping NTC4 for a oil pressure sensor if it can be accommodated.
* [x] Power from Engine supply since oil pressure sensors use > 10mA typically. This means fuel levels and temperature will only be available with engine controls turned on.
* [x] Check voltage calibration, some doubt over digtal multi meter accuracy used previously - verified against a ADS1115 to be accurate
* [x] Add firmware version, shows on the help, press h when connected over serial.
* [ ] Create a attiny3226 based board, with 12bit ADCs for better resolution on the voltages.
* [ ] Install onboard

# Could do... but probably will not

uses about 50mA when running, but could probably reduce by powering most of the sensors by pulling an IO pin down which would mean NTCs, Fuel and perhaps other ADC dividers would only draw current when being read. However this might safe 5mA and when the engine is running, 8mA is irrelevant. The Fuel sensor is powered of 5v and draws 5mA which is the worst offender. Temperature sensors are 10K NTCs drawing 0.5mA each. Engine coolant sensor is powered by the engine so not relevant and the comparitor chip is idle when the engine is not running. Could power this down when there is no engine battery voltage indicating the engine controls are on. TBH, not sure its wroth optimising power as all the instruments draw about 2.5A currently (including chart plotter) so this represents +2% drain, which is less than adjusting the brightness on the OLED displays.






