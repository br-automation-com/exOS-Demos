#!/bin/sh

finalize() {
    cd ..
    rm -rf build/*
    rm -rf node_modules/*
    rm -f Makefile
    sync
    exit $1
}

mkdir build > /dev/null 2>&1
rm -f *.deb

npm install
if [ "$?" -ne 0 ] ; then
    cd build

    finalize 2
fi

mkdir -p node_modules #make sure the folder exists even if no submodules are needed

rm -rf build/*
cd build

cmake -Wno-dev ..
if [ "$?" -ne 0 ] ; then
    finalize 4
fi

cpack
if [ "$?" -ne 0 ] ; then
    finalize 4
fi

cp -f exos-comp-framework_1.0.0_amd64.deb ..

finalize 0
