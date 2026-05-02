# tbdsfs01
TBDFS version 0.1

This is a proof of concept for the To Be Determined File System

## Instructions
Put a bunch of file into a directory.  $tbd-dir

Run 'tbdfs -s $tbd-dir'

This will make a 'meta' directory and fill it with text files matching the files in $tbd-dir

Additional metadata can be added to those files.  the format is;

attribute/type/key

Attribute can be an string under 32 bytes and no '/', and Key can be 255 bytes with no '/'.

The types are;
text = 256byte string
integer = signed 64bit integer
decimal = double floating point
number = usigned 64bit integer

title/text/Alice in Wonderland
year/integer/1983

Then run 'tbdfs -b $tbd-dir'

by default it posts to /srv/tbdfs and mounts to /n/tbd

## Use

'ls /n/tbd' outputs;
/n/tbd/index  <- walks attribute/key
/n/tbd/meta  <- outputs metadata for file
/n/tbd/pool  <- flat list of files
/n/tbd/tree  <- walks B+ tree of attribute
/n/tbd/ℚ  <- takes direct atribute/key pair

ls '/n/tbd/ℚ/arist/Pink Floyd' will return every file with the metadata 'artist/text/Pink Floyd'

## Bugs
Probably.  This is just an early proof of concept.
