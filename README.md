# Final project for BSc Computer Science

This project is to fulfill the Final Project requirements for the University of London BSc Computer Science course.

## Deploy to ESP32 (taken from https://github.com/espressif/esp-tflite-micro)

The following instructions will help you build and deploy this sample
to [ESP32](https://www.espressif.com/en/products/hardware/esp32/overview)
devices using the [ESP IDF](https://github.com/espressif/esp-idf).

The sample has been tested on ESP-IDF version `release/v5.1.2`  with the following devices:
- LilyGo T-Camera-Plus

### Install the ESP IDF

Follow the instructions of the
[ESP-IDF get started guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html)
to setup the toolchain and the ESP-IDF itself.

The next steps assume that the
[IDF environment variables are set](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html#step-4-set-up-the-environment-variables) :

 * The `IDF_PATH` environment variable is set
 * `idf.py` and Xtensa-esp32 tools (e.g. `xtensa-esp32-elf-gcc`) are in `$PATH`

### Dependencies

This example requires an external component `esp32-camera` which is the submodule of the example.
If you have not cloned current repo with `--recursive` option, please use `git submodule update` to get it cloned.

### Building the example

Set the chip target (For esp32s3 target, IDF version `release/v4.4` is needed):

```
idf.py set-target esp32
```

Then build with `idf.py`
```
idf.py build
```

### Load and run the example

To flash (replace `/dev/ttyUSB0` with the device serial port):
```
idf.py --port /dev/ttyUSB0 flash
```

Monitor the serial output:
```
idf.py --port /dev/ttyUSB0 monitor
```

Use `Ctrl+]` to exit.

The previous two commands can be combined:
```
idf.py --port /dev/ttyUSB0 flash monitor
```
