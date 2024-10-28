## Uvcvideo is not working at his best on the raspberry
Due to the usb controllers and some usbcamera reporting wrong 
speeds to uvcvideo a kernel patch is needed to make everithing work fine.
It's a known problem of the raspberry pi and some other linux devices.
Read more [here](https://www.thegoodpenguin.co.uk/blog/understanding-why-usb-isochronous-bandwidth-errors-occur/) 
and [here](https://www.thegoodpenguin.co.uk/blog/multiple-uvc-cameras-on-linux/), the patch is taken from 
the second one ([kernel lore](https://lore.kernel.org/linux-media/20200821220038.16420-1-amurray@thegoodpenguin.co.uk/)).
Related resources:
- [raspberry pi linux github](https://github.com/raspberrypi/linux/issues/238), merged
- [raspberry pi linux github](https://github.com/raspberrypi/linux/issues/3406), wrong documentation

The patch mentioned needs small adjustements in order to be applyed, a modified version is provided here (in this folder).
Relative to kernel 6.12 (raspberry pi's custom kernel).

To understand how to patch a kernel module just ask chatGPT, you don't need to rebuild the entire kernel.

## Module parameters

Some uvcvideo's parameters can be set in order to improve the performance:
- nodrop=1 keeps all the frames, better results without the patch but sometimes the resulting
jpegs are corrupted (not too much, just some horizontal grey lines)
- timeout=5000 everybody does this, idk
- bandwidth_cap=700 available only after the kernel patch. Use [this site](https://www.planet.com.tw/en/tools/camera-bandwidth-calculator)
to compute the camera bitrate, then use this formula: bandwidth_cap >= bitrate / 8 / 8 * 1000. The first 8 is for converting from bits to bytes,
the second (and the 1000) is for conversion between seconds and microframes (there are 8 microframes per milliseconds).
