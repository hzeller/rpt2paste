**Note, this project is now superseded by https://github.com/hzeller/rpt2pnp
which does solder dispensing as well as pick and placing**
  
KiCAD RPT file to G-Code for didspensing solder paste.

There are probably better ways. This is mostly experimental, working with a solder-paste dispenser
hooked up to the fan-output of https://github.com/jerkey 's Type-A 3D printer.

The bumps.rpt file is for testing, created from https://github.com/hzeller/bumps project.

lead solder paste (zephpaste), 27gauge needle (grey), 60 PSI, -d 250 -D 3024
lead solder paste (zephpaste), 0.78mm needle (teal), 60 PSI, -d 235.40 -D 132.74

![Postscript Output][route]

[route]: https://github.com/hzeller/rpt2paste/raw/master/img/bumps-paste.png
