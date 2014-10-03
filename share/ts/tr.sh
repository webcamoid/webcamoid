#!/bin/sh

GLOBIGNORE="*.sh *.qm"
langs=$(ls --ignore=\.sh | grep -v \.sh | grep -v \.qm | sed "s/.ts$//")

echo $langs

for lang in $langs
do
    lrelease-qt5 -removeidentical -compress ${lang}.ts -qm ${lang}.qm
done
