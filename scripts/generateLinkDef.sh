#!/bin/bash
#we should be called from the top-level daqman directory

#print all the boilerplate stuff
echo "#ifdef __MAKECINT__"
echo "#pragma link off all globals;"
echo "#pragma link off all classes;"
echo "#pragma link off all functions;"
echo "#pragma link C++ nestedclasses;"
echo "#pragma link C++ global functions;"

#find all the header files
headers=$(find */include -name "*.hh" -o -name "*.h")
headers=$(echo $headers | grep -v "doc" | grep -v "libdaqman")
stl=""

for file in $headers ; do
    [ -n "$(grep -e ClassDef -e RQ_OBJECT $file)" ] || continue
    echo "#pragma link C++ defined_in \"$file\";"
    #now look for template declarations
    exclude=0
    while read line ; do
        #don't look in places hidden from CINT!
	[ -n "$(echo $line | grep '#ifndef __CINT__')" ] && (( exclude++ ))
	[ -n "$(echo $line | grep '#ifndef __MAKECINT__')" ] && (( exclude++ ))
	[ -n "$(echo $line | grep '#endif')" ] && (( exclude-- ))
	
	[ $exclude -lt 0 ] && exclude=0
	[ $exclude -gt 0 ] && continue

        #templates may have spaces, so sub a special character for them
	templates=$(echo $line | grep -who "std::.*<.*>" | sed 's/ /%/g')
	[ -n "$templates" ] && stl="$stl $templates"
    done <$file
done
echo ""

#stl=$(echo $stl | sed 's/ /\n/g' | sort -u)
#for line in $stl ; do
#    echo "#pragma link C++ class $line+;" | sed 's/%/ /g'
#done

#echo "#pragma link C++ class std::pair<std::string, std::string>+;"
#close the ifdef
echo "#endif /*__MAKECINT__*/"
