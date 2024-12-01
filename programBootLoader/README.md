

# Program an Arduino UNO with Arduino ISP 

This will program a 328p with a urboot bootloader from https://github.com/MCUdude/MiniCore where previously there was none. Once the bootloader is used, standard programming over UART can be used. Fuses are also set here, see platform.ini here for more details.

## Connect

* Uno 10uF between RST and GND to stop it reseting while programming
* Uno -> Target
* 0V -> 0V
* 5V -> 5V
* D10 -> ISP Reset
* D11 -> MOSI
* D12 -> MISO
* D13 -> SCK

run


    pio run -e fuses_bootloader_uno_isp -t bootloader

output should end with....

        ....
        avrdude: verifying ...
        avrdude: 1 bytes of lock verified

        avrdude: safemode: lfuse reads as F7
        avrdude: safemode: hfuse reads as D6
        avrdude: safemode: efuse reads as FC
        avrdude: safemode: Fuses OK (E:FC, H:D6, L:F7)

        avrdude done.  Thank you.

        =========================================================================== [SUCCESS] Took 3.98 seconds ===========================================================================

        Environment               Status    Duration
        ------------------------  --------  ------------
        fuses_bootloader_uno_isp  SUCCESS   00:00:03.983
        =========================================================================== 1 succeeded in 00:00:03.983 ===========================================================================

Then the LED on SCK should be flashing. The default program is blink.

Now the board can be programmed over UART as normal. The bootloader is urboot

# Errors

If avrdude reports

    avrdude: stk500_recv(): programmer is not responding

Then check the 10uF cap is connected properly (or the wrong port is used)

If avrdude reports

    avrdude: Device signature = 0x000000
    avrdude: Yikes!  Invalid device signature.
             Double check connections and try again, or use -F to override
             this check.

Try swapping MOSI and MISO

