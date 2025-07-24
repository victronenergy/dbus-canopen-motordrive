#!/bin/bash

. /opt/venus/scarthgap-arm-cortexa8hf-neon/environment-setup-cortexa8hf-neon-ve-linux-gnueabi
(cd /workspace/dbus-canopen-motordrive && ../velib/mk/init_build.sh && make)