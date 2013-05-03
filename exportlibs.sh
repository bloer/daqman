#!/bin/sh
dir=libdaqman
make distclean
make -j2 libs
mkdir -p $dir/include
heads="`find . -name '*.hh' `"
cp $heads $dir/include/
mkdir -p $dir/lib
cp lib/lib* $dir/lib/
