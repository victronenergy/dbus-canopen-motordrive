#!/bin/bash

rm -rf ./venus-data-tmp
mkdir ./venus-data-tmp

cp -r ./archive/* ./venus-data-tmp
cp obj/linux-arm-gnueabi-release/dbus-canopen-motordrive ./venus-data-tmp/dbus-canopen-motordrive

mkdir -p ./dist
rm -f ./dist/venus-data.zip

(cd ./venus-data-tmp && zip -r ../dist/venus-data.zip ./*)

rm -rf ./venus-data-tmp