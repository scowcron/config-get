config-get
==========

A utility for reading values from a libconfig-style configuration file.

Usage
=====

	config-get [opts] key

options:

	-h, --help  show this help text and exit
	-f, --file  path to the configuration file to manipulate
				if this is not specified, use standard input and output
				to read/write data, respectively.
	-t, --type  the type of expected data for read or manipulated data.
				if reading, a warning will be displayed if the data type does
				not match. if writing, the system will error.
				possible types are: number, boolean, string.

