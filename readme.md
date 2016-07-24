# Movled
### Display movies on a grid of LEDs

Movled reads movies and outputs them to a Entex DMX box and finally to a White Wing Logic RGB48 dimmer. The movies can be triggered by MIDI.

# Movies

List the movies you want to use in `data/movies.csv`

# Settings

In `data/settings.json` you can change the following:

- **numPixelsPerLED**: This is the step when looking at the movies, i.e. if you use a 2x2 group of pixels to represent LEDs, this value should be 2.
- **output**: The dimensions of your LED output grid.
- **wiring**: Options `rowMajor` or `coloumnMajor`, this is how your LEDs are wired
- **movieOrigin**: The origin of your LEDs with respect to the movieOrigin
- **midiChannel**: The MIDI channel to listen on, `0` means all channels.
- **constantRefresh**: If this is true, the settings are realoaded every second or so.
