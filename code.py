import os
import board
import busio as io
import digitalio
import storage
import adafruit_sdcard
import microcontroller
from time import sleep
from lcd.lcd import LCD, CursorMode
from lcd.i2c_pcf8574_interface import I2CPCF8574Interface

i2c = io.I2C(board.GP5, board.GP4, frequency=100000)
lcd = LCD(I2CPCF8574Interface(i2c, 0x27), num_rows=4, num_cols=20)
lcd.set_cursor_mode(CursorMode.HIDE)

# Use any pin that is not taken by SPI
SD_CS = board.GP13

# Connect to the card and mount the filesystem.
spi = io.SPI(board.GP10, board.GP11, board.GP12)
cs = digitalio.DigitalInOut(SD_CS)
sdcard = adafruit_sdcard.SDCard(spi, cs)
vfs = storage.VfsFat(sdcard)
storage.mount(vfs, "/sd")

with open("/sd/testfile.txt", "w") as writefile:
    print("First Line", file=writefile)
    writefile.close()

with open("/sd/testfile.txt", "a") as appendfile:
    print("Second Line", file=appendfile)
    appendfile.close

with open("/sd/testfile.txt", "r") as inputfile:
    for line in inputfile:
        print(line)
    inputfile.close

while True:
    with open("/sd/temphistory.txt", "a") as appendfile:
        print(microcontroller.cpu.temperature, file=appendfile)
    appendfile.close
    sleep(5)