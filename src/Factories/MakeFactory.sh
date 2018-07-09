#! /bin/sh
#Such Inception
#First arg must contain the path to ./ from the caller
cd $1
../../tools/EraseComments.sh Interfaced.txt InterfacedNoComments.txt

sed "s/.*/	DEF(&)\;/" InterfacedNoComments.txt > headers.txt
# elt -> DEF(elt);
sed "s/.*/			&::registerFactory()\;/" InterfacedNoComments.txt > registration.txt
cat head.txt headers.txt middle.txt registration.txt tail.txt > ../Factories.cpp

rm headers.txt registration.txt InterfacedNoComments.txt
