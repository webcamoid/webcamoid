#!/bin/sh

git shortlog -se | cut -c8- | sed '/hipersayan.x/d' | sort > StandAlone/share/COLLABORATORS.txt
