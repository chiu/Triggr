#!/bin/bash

echo "Autoconfing"
cd triggr
autoconf
cd ..

echo "Copying stuff to form valid package in cleanPkg/"
rm -rf cleanPkg
mkdir cleanPkg
mkdir cleanPkg/triggr
mkdir cleanPkg/triggr/{man,R,src}

cp -r triggr/man/*.Rd cleanPkg/triggr/man/.

cp -r triggr/R/*.R cleanPkg/triggr/R/.
cp -r triggr/src/*.c cleanPkg/triggr/src/.
cp -r triggr/src/*.h cleanPkg/triggr/src/.
cp -r triggr/src/*.in cleanPkg/triggr/src/.

cp triggr/DESCRIPTION cleanPkg/triggr/.
cp triggr/INSTALL cleanPkg/triggr/.
cp triggr/NAMESPACE cleanPkg/triggr/.
cp triggr/configure cleanPkg/triggr/.

echo "Copying done."

