cd /aptrepo/
dpkg-scanpackages -m binary /dev/null | gzip -9c > binary/Packages.gz
dpkg-scansources source /dev/null | gzip -9c > source/Sources.gz
