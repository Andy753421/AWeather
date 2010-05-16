#!/bin/bash

#Grab command line arguments
OLDVERSION=$1
VERSION=$2

#Local
SRCSFOLDER=~/pkgbuild

#Remote
APTREPO=/aptrepo
USER=compuwizard123
HOST=compuwizard123.dyndns.org

#build using 2 threads
BUILD="debuild -j2 -uc -us"

#look for new libgis version
APP=libgis
cd $SRCSFOLDER/$APP/
uscan --download-version $VERSION
cd $SRCSFOLDER/$APP/${APP}-$OLDVERSION/
uupdate -v $VERSION ../${APP}-$VERSION.tar.gz
cd $SRCSFOLDER/$APP/${APP}-$VERSION/
cp $SRCSFOLDER/$APP/${APP}-$OLDVERSION/debian/watch debian/

#build libgis and libgis-dev
$BUILD
dchroot -d "linux32 $BUILD -ai386"

#copy libgis and libgis-dev to remote repo
scp ../${APP}_$VERSION-0ubuntu1.diff.gz ../${APP}_$VERSION-0ubuntu1.dsc ../${APP}_$VERSION.orig.tar.gz $USER@$HOST:$APTREPO/source/
scp ../${APP}_$VERSION-0ubuntu1_amd64.deb ../${APP}-dev_$VERSION-0ubuntu1_amd64.deb ../${APP}_$VERSION-0ubuntu1_i386.deb ../${APP}-dev_$VERSION-0ubuntu1_i386.deb $USER@$HOST:$APTREPO/binary/

#install updated libgis and libgis-dev before building aweather
sudo dpkg --install ../${APP}_$VERSION-0ubuntu1_amd64.deb
sudo dpkg --install ../$APP-dev_$VERSION-0ubuntu1_amd64.deb
dchroot -d "linux32 sudo dpkg --install ../${APP}_$VERSION-0ubuntu1_i386.deb"
dchroot -d "linux32 sudo dpkg --install ../$APP-dev_$VERSION-0ubuntu1_i386.deb"

#look for new aweather version
APP=aweather
cd $SRCSFOLDER/$APP/
uscan --download-version $VERSION
cd $SRCSFOLDER/$APP/${APP}-$OLDVERSION/
uupdate -v $VERSION ../$APP-$VERSION.tar.gz
cd $SRCSFOLDER/$APP/$APP-$VERSION/
cp $SRCSFOLDER/$APP/${APP}-$OLDVERSION/debian/watch debian/

#build aweather
$BUILD
dchroot -d "linux32 $BUILD -ai386"

#copy aweather to remote repo
scp ../${APP}_$VERSION-0ubuntu1.diff.gz ../${APP}_$VERSION-0ubuntu1.dsc ../${APP}_$VERSION.orig.tar.gz $USER@$HOST:$APTREPO/source/
scp ../${APP}_$VERSION-0ubuntu1_amd64.deb ../${APP}_$VERSION-0ubuntu1_i386.deb $USER@$HOST:$APTREPO/binary/

#update remote repo
ssh $USER@$HOST '~/pkgbuild/aptrepo.sh'
