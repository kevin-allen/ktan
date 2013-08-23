/****************************************************************
Copyright (C) 2013 Kevin Allen

This file is part of kacqtan.

kacq is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
kacq is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with kacq.  If not, see <http://www.gnu.org/licenses/>.

File with declarations of the main structures and functions used in kacq


****************************************************************/
#include <stdio.h>
#include <fcntl.h> // for file operations
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h> // for the nanosleep
#include <pthread.h> // to be able to create threads
#include <math.h>
#include <pwd.h> // to get the home directory as default directory
#include <stdlib.h>
#include <getopt.h>
#include <gtkmm.h>
#include <cairo.h>
#include "../config.h"
#include <iostream> 
