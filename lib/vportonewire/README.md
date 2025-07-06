
This is a version of OneWire coped from https://github.com/SpenceKonde/OneWire that uses the Vports. The reason for doing this is when the OneWiree Pin is PC1, the original library does not work.

So switched to using a vport directly, which seems to fix the problem.

The downside of this approach is that the OneWire Pin must be a compile time define. Since this is harware wiring thats not a problem for me.

    -D ONE_WIRE_PIN=11   

ie PIN_PC1 on an Attiny3226




