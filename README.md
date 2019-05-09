# ktan

ktan is a C++ program to perform data acquisition with the GNU/Linux operating system. This program was developed to work with the Evaluation board from Intan Technologies. The low level code to interface with the board comes from the RHD2000 interface software of Intan Technologies. The visual interface of kacq was written using the gtk (gtkmm) library. We have run and tested ktan on Ubuntu 14 to 18 LTS.

# Features

* Record up to at least 64 channels (depending on hardware).
* Rewind the oscilloscope to re-examined recently acquired signals.
* Independent threads for acquisition, recording and oscilloscope to take advantage of multi-core processors.
* Control recording and visualize data from a remote computer via ssh (-X).

# Tests

* We could record with ktan for periods of 15 hours without problems.

# Install

The installation notes are for a computer running Fedora 19 or above.

Get the source code from the github directory: `git clone https://github.com/kevin-allen/ktan.git`

Install the gtkmm30-devel package for your computer.
Fedora: `yum install gtkmm30-devel.x86_64`
Ubuntu: `sudo apt-get install libgtkmm-3.0`

Go into the source directory and run: `./autogen.sh;./configure; make; su -; make install`

# Note on installation with Fedora >=25

With these Fedora versions, the oscilloscope does not work out of the box. This appears to be because Fedora switched from Xorg to Wayland. To fix the issue, you can try the following in a terminal:

`sudo nano /etc/gdm/custom.conf`

Then remove the hash in this line
 
`WaylandEnable=false`

Restart the computer and Fedora will use Xorg instead of Wayland. The oscilloscope should now work.


# More information

Documentation is available as a [wiki page](https://github.com/kevin-allen/ktan/wiki)
