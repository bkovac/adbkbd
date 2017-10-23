# adbkbd - bitbanged ADB kernel module
This is a simple work-in-progress kernel module aiming to bring ADB (Apple Desktop Bus) support to your embedded Linux board. It uses busy-waiting delays for bitbanging the data (they don't last longer than 2ms, and happen every 15ms) and an interrupt based state machine for receiving it. It is pretty low power and can be ran on a Raspberry Pi Zero without any problems. Only generic linux gpio functions are used so you could probably run it on any embedded Linux board which has them implemented. It is also dependant on hrtimers but they don't really need to be that precise.

## Hardware setup
This is pretty straight-forward and requires only a few wires and a 3v3-5v level converter. You could get one like [this](https://www.adafruit.com/product/757) from Adafruit or any other electronics store. I, however, went with the cheaper method and built one like [this](http://www.hobbytronics.co.uk/image/data/tutorial/mosfet_level_converter.jpg) using only one MOSFET and two resistors.

You need to connect the ADB connector's VCC and GND to your power source, and the ADB data line through the level converter to TWO pins on your Linux board. I say two because I'm lazy and this was easier than doing it the proper way.

## Installation
Implying you have your kernel source downloaded, just type this:
```
make
sudo insmod adb.ko
```

## ToDo
- Add support for more devices than just a keyboard
- Make the attention pulse use a non-blocking delay
- Clean the code up a little
- Make the module use only one GPIO, instead of two
