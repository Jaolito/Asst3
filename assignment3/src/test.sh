#!/bin/bash

make clean
make
fusermount -u /tmp/apm145/mountdir/
rm -rf /tmp/apm145/mountdir
rm /tmp/apm145/drivefile
mkdir /tmp/apm145/mountdir
touch /tmp/apm145/drivefile
./sfs /tmp/apm145/drivefile /tmp/apm145/mountdir
