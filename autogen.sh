#!/bin/sh

PKG_NAME="libindicator"

which gnome-autogen.sh || {
	echo "You need gnome-common from GNOME SVN"
	exit 1
}

USE_GNOME2_MACROS=1 \
. gnome-autogen.sh
