#!/bin/bash

rm -rf ./venus-data-tmp
mkdir ./venus-data-tmp

cp -r ./archive/* ./venus-data-tmp
cp obj/linux-arm-gnueabi-release/dbus-sevcon ./venus-data-tmp/dbus-sevcon

mkdir -p ./dist
rm -f ./dist/venus-data.zip

(cd ./venus-data-tmp && zip -r ../dist/venus-data.zip ./*)

rm -rf ./venus-data-tmp