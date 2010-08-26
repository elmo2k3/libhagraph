#!/bin/bash

# Achtung: das Wallpaper sollte am besten schon die Auflösung des Desktops haben!

BASE_DIR="/home/bjoern/hagraph_desktop" # muss erstellt werden, darf leer sein
RESOLUTION="-x 800 -y 300" # auflösung des graphen
WALLPAPER="/home/bjoern/hagraph_desktop/foo.png" # wallpaper das überlagert werden soll
POSITION=NorthEast # position des graphen auf dem wallpaper

hagraph -g 3 -h 1 -i 0 -j 0 -k 4 -l 0 $RESOLUTION -z $BASE_DIR/graph.png -u
convert $WALLPAPER -alpha on $BASE_DIR/graph.png -gravity $POSITION -geometry +40+40 -composite -format png -quality 90 $BASE_DIR/background.png
gconftool-2 --type string -s /desktop/gnome/background/picture_filename "$BASE_DIR/background.png"
