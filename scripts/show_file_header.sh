#!/bin/sh

if [ $# -ne 1 ] || [ ! -f "$1" ] ; then
    echo "Usage: $0 <rawfile>"
    exit 1
fi
FILE=$1

MAGICNUM=$(hexdump -n 4 -e '1/4 "%08x"' $FILE)
echo $MAGICNUM

if [ "$MAGICNUM" != "dec0ded1" ] ; then
    echo "$FILE does not appear to be a daqman raw file!"
    echo "Expected 0xdec0ded1 in first 4 bytes; got 0x$MAGICNUM"
    exit 2
fi

echo "Header for file $FILE:"
echo "--------------------------------"
echo "Magic number:          $MAGICNUM"
echo "Global header size:    $(hexdump -s 4 -n 4 -e '1/4 "%u"' $FILE)"
echo "Global header version: $(hexdump -s 8 -n 4 -e '1/4 "%u"' $FILE)"
echo "Event header size:     $(hexdump -s 12 -n 4 -e '1/4 "%u"' $FILE)"
echo "Event header version:  $(hexdump -s 16 -n 4 -e '1/4 "%u"' $FILE)"
echo "File size:             $(hexdump -s 20 -n 4 -e '1/4 "%u"' $FILE)"
echo "Start time:            $(hexdump -s 24 -n 4 -e '1/4 "%u"' $FILE)"
echo "End time:              $(hexdump -s 28 -n 4 -e '1/4 "%u"' $FILE)"
echo "Run ID:                $(hexdump -s 32 -n 4 -e '1/4 "%u"' $FILE)"
echo "File index in series:  $(hexdump -s 36 -n 4 -e '1/4 "%u"' $FILE)"
echo "Number of events:      $(hexdump -s 40 -n 4 -e '1/4 "%u"' $FILE)"
echo "Min Event ID:          $(hexdump -s 44 -n 4 -e '1/4 "%d"' $FILE)"
echo "Max Event ID:          $(hexdump -s 48 -n 4 -e '1/4 "%u"' $FILE)"

