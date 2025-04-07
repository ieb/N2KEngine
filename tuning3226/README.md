# Tuning Attiny3226 clock

This is required to get accurate RPM radings. The clock on the Attiny is stable and accurate once tuned.

## Process

In the parent directory burn the fuses to set the clock speed of the Attiny3226 to 16MHz

    cd ../
    vi platform.ini # to set the serial ports 
    pio run -e attiny3226 -t fuses


upload tuningSource to a Uno which will produce a 500Hz square wave on pin 9. The Uno must have a genuine crystal clock source, not a resonator.

    cd tuningSource
    pio run -e uno -t upload

Connect pin 999 to PB1 (Oil pressure signal) on the board.

Upload megaTinyTuner and monitor the serial line to ensure check the process completes.

    cd megaTinyTuner
    pio run -e attiny3226 -t upload


Output from the attiny3226 will be something like the following possibly with different numbers.


    tuning with new sketch
    25:35:53:71:78:80
    tuning
    Dump of USERSIG after tuning
    25:35:53:71:78:80
    Tuning should now be complete.


Once tuned, the main program can be uploaded
