## SystemD config

```
[Unit]
Description=mandeye_controllel

[Service]
User=mandeye
WorkingDirectory=/home/mandeye/mandeye_controller/build
ExecStart=/home/mandeye/mandeye_controller/service/start_mandeye_controllel.sh
Restart=always
StandardInput=/dev/null
StandardOutput=/dev/null

[Install]
WantedBy=multi-user.target

# /etc/systemd/system/mandeye_controllel.service.d/override.conf
[Service]
Environment="MANDEYE_CAMERA_IDS=0 2"
Environment="IGNORE_LIDAR_ERROR=1"
StandardInput=journal
StandardOutput=journal
```

## Static ip for livox

> /etc/dhcpcd.conf
```

interface eth0
static ip_address=192.168.1.5/24
static routers=
static domain_name_servers=
static domain_search=
```

## Usbmount

Remember: no sync on mount

```
FILESYSTEMS="vfat ext2 ext3 ext4 hfsplus ntfs fuseblk"
FS_MOUNTOPTIONS="-fstype=vfat,users,rw,umask=000 -fstype=exfat,users,rw,umask=000"
VERBOSE=yes
```

## Udevd

Edit udevd config and add:
```
[Service]
PrivateMounts=no
```

## Gpio

`sudo usermod -a -G gpio $USER`


