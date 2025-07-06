#!/bin/sh

script="install dbus-sevcon service"

echo "### ${script} starting"

ln -s /data/dbus-sevcon/ /opt/victronenergy/service/dbus-sevcon

echo "### ${script} done"

exit 0