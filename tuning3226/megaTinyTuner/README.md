# Mega Tiny Tuner

Copied from https://github.com/SpenceKonde/megaTinyCore/tree/master/megaavr/libraries/megaTinyCore/examples/megaTinyTuner to adjust for this board and make work with platformIO as used here.

NOte the source needs to be 500Hz with exactly 50% duty cycle so that the pulse width is exactly 1ms for both high and low pulses. Anything else wont work.

# Modifications

* Need to use a pin that is exposed, PB1 looks good as it is the Oil Pressure sensor sense pin, with only a 10nF cap to ground. There is a protective diode to +5V, but nothing else. Hopefully the 10nF will not impact the rise time of the pulse.

* FORCE_PIN PA6 is connected to Fuel level which is normally pulled high and can be pulled low if required. No change here.

* Serial debug output modified to give clearer feedback .... wasnt sure it worked first time. 

* added platformio.ini to fit with my dev flow and programers.

