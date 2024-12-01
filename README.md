# N2K Engine Monitor

This monitors an engine, mostly temperatures and fuel, emitting suitable N2K messages. Targeting a Volvo Penta D2-40. Code and pcb are based on the interface board in https://github.com/ieb/EngineManagement, however this time the pcb is from scratch using a Atmel 328p directly to make the unit more robust. The original has been in service for 3 years with no problems seen, outlasting the Volvo Penta MDI unit, which failed 2x in < 4 years. Ok, the replacement was based on relays following the Perkins schematic for the same engine, so not really hard to do.

# PCB

Single board. If using a 328p chip you will need to flash the bootloader. See programBootLoader for details. 

## Changes from original

* All components SMD, no draughtboards or modules with pin headers to fail.
* Switched from a LM385 to a LMV393 for the flywheel pulse generator as all I need is a clean square wave to count the pulses. (actually I miss ordered 10 LMV393's and then switched over).
* Changed to a 16MHz crystal on the MCP2515 since this is what the 328p is using. Perhaps 1 crystal would work for both, but probably not.


# Todo

* [x] build board.
* [x] Flash bootloader and test
* [ ] Port code from original
* [ ] Test and Calibrate NTCs
* [ ] Test and Calibrate Coolant and fuel
* [ ] Test On N2K Bus
* [ ] 






