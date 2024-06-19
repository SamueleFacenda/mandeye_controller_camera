- since gpioClientPtr is necessary for going from resource_waiting to idle (and app running) is probably not necessary
to check if it's not empty before every led set.
- also if there are not strange states at runtime it should be safe to not reset the leds at avery state change.
- convert if-else in state management to switch