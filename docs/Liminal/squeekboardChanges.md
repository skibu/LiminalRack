# Squeekboard Virtual Keyboard Software Changes

Squeekboard is the standard virtual keyboard for Raspberry Pis. Had a great deal of trouble with getting 
the keyboard to be displayed over a full screen window, and really want Liminal Rack to be display 
full screen so that don't get window decorations or other things that yell COMPUTER! Turns out this is
a previously encountered problem. 

At first I tried making the Liminal window maximized instead of full screen, as some people recommended. 
Got this mostly working, but could never get rid of the window decorations since using Wayland in order
to handle touchscreen input. Therefore needed another solution.

The other known solution is to modify the source code for Squeekboard and deploy it. Turns out this is
actually pretty difficult to do. Therefore I have gathered the steps on how  on a Raspberry Pi 
exactly how to modify and deploy the source code.

## Problem and solution reported by others
Problem and solution described at https://forums.raspberrypi.com/viewtopic.php?p=2327148#p2330371
labwc supports the following layer order:
* background
* bottom
* regular application windows
* always-on-top windows
* top
* fullscreen windows
* overlay
* lockscreen

By default, Squeekboard is hardcoded to appear on the top layer.
However, if you want the keyboard to appear above fullscreen apps or Chromium in kiosk mode, it really should be placed on the overlay layer instead.

The file to edit: eek/layersurface.c

Looking for function: void phosh_layer_surface_set_layer (PhoshLayerSurface *self, guint32 layer)

Add directly after the" priv = ... Block" following code:
/* Check env var override */
const char *env = g_getenv("SQUEEKBOARD_LAYER");
if (env && g_strcmp0(env, "overlay") == 0) {
layer = 3; // ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY

## Cryptic Instructions on recompiling Squeekboard on Raspberry Pi
I found these instructions, which are correct but not complete:

https://github.com/raspberrypi-ui/squeekboard/issues/10

```
# Download the squeekboard source package (you'll need deb-src enabled in sources.list)
$ apt source squeekboard
# Parse the Debian control file and install all build dependencies
$ sudo mk-build-deps -i ./squeekboard-1.21.0/debian/control
# Change into the source directory
$ cd squeekboard-1.21.0
# Perform a binary build of the package (skipping signing)
$ dpkg-buildpackage -b -uc -us
# Install your package
$ sudo dpkg -i ../squeekboard_1.21.0-1+rpt11_arm64.deb
```

## Complete Instructions

### Updating sources.list:
Good instructions at https://www.raspberrypi.com/documentation/computers/software-sources.html 

For some systems it is in /etc/apt/sources.list
But for raspberry pi actually in /etc/apt/sources.list.d/raspi.sources 
Originally the file contains just:
```
Types: deb
URIs: http://archive.raspberrypi.com/debian/
Suites: trixie
Components: main
Signed-By: /usr/share/keyrings/raspberrypi-archive-keyring.pgp
```
Change the Types to include deb-src, as in “deb deb-src”:
```
Types: deb deb-src
URIs: http://archive.raspberrypi.com/debian/
Suites: trixie
Components: main
Signed-By: /usr/share/keyrings/raspberrypi-archive-keyring.pgp
```
Then update the package lists: 
```
$ sudo apt update
```

### Downloading software

Fetch the source code into the current directory:
```
$ apt source squeekboard
```
When I did this on 2/18/26 I got directory squeekboard-1.43.1 

Next, you need to install a helper package (which loads a very large number of packages):
```
$ sudo apt install devscripts
```

Change into the new source code directory:
```
$ cd squeekboard-1.43.1 
```

### Modifying the software and Compiling
Parse the Debian control file and install all build dependencies
```
$ sudo mk-build-deps -i debian/control
```

Attempt a binary build of the package (skipping signing) to make sure you can compile everything. 
This can take a couple of minutes. You will likely get an error.
```
$ dpkg-buildpackage -b -uc -us
```

Fix the source code. First fix the “name” compile error.
Add name: None, to line 865 after scale, in function scaling_test_wide in src/state.rs it should compile.
```
  scale,
  name: None,
```

Now fix the z-index of squeekboard so that it will be displayed on top of even full screen windows.
In eek/layersurface.c phosh_layer_surface_set_layer (PhoshLayerSurface *self, guint32 layer just before 
layer is used on line 771,
```
  // Always use overlay layer so that keyboard displayed over even fullscreen windows
  layer = 3; // ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY
```

Now you should be able to compile the corrected squeekboard software:
```
$ dpkg-buildpackage -b -uc -us
```

Confirm that you have the expected resulting package file:
```
$ ls -l ../squeekboard_1.43.1-1+rpt1_arm64.deb
...
-rw-r--r-- 1 liminal liminal 2469588 Feb 18 14:34 ../squeekboard_1.43.1-1+rpt1_arm64.deb
```

### Installing the fixed version of Squeekboard

Install the newly created package
```
$ sudo dpkg -i ../squeekboard_1.43.1-1+rpt1_arm64.deb
...
(Reading database ... 181918 files and directories currently installed.)
Preparing to unpack .../squeekboard_1.43.1-1+rpt1_arm64.deb ...
Unpacking squeekboard (1.43.1-1+rpt1) over (1.43.1-1+rpt1) ...
Setting up squeekboard (1.43.1-1+rpt1) ...
Processing triggers for gnome-menus (3.36.0-3) ...
Processing triggers for mailcap (3.74) ...
Processing triggers for desktop-file-utils (0.28-1) ...
Processing triggers for libglib2.0-0t64:arm64 (2.84.4-3~deb13u2) ...
```
Log file for dpkg is at /var/log/dpkg.log but doesn’t seem to be very helpful

The above dbpg command installs squeekboard from the `.deb` file into `/usr/bin/squeekboard` . 
It also updates the us_wide.yaml file used to specify the keyboard layout. Therefore you
must copy in your custom us_wide.yaml file to `/usr/share/misc/squeekboard/keyboards/us_wide.yam`
after the modified squeekboard software has been installed.

