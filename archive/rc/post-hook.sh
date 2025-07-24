#!/bin/sh

script="install dbus-canopen-motordrive service"

echo "### ${script} starting"

ln -s /data/dbus-canopen-motordrive/ /opt/victronenergy/service/dbus-canopen-motordrive

echo "### ${script} done"

exit 0