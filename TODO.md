- since gpioClientPtr is necessary for going from resource_waiting to idle (and app running) is probably not necessary
to check if it's not empty before every led set.
- also if there are not strange states at runtime it should be safe to not reset the leds at every state change.
- convert if-else in state management to switch
- use video compression for images saving instead of single images saving. Reduce ram usage and write space (images are 6Mb
each, 10 images per second is 60Mb/s, usb speed is 45Mb/s avg).


Problems:
- Write 10 imgs/s is not doable
- Not-raw video codecs like h264 takes +10s to compress a 5s video (10fps)
- Mayber jpeg images are the solution
- We lose all the possible compression from continous images (video compression is
way better in this case)
- We lose quality (probably)
- Jpeg compression time: 0.05s, final size: ~250KB => (10fps) 2.5MB/s write, 0.5s encode

