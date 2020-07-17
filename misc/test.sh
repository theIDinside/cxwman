#!/bin/bash

# Remove any unlink()'ed unix sockets, if they exist
rm cxwman_ipc

# Script for starting Xephyr & CXWM
XEPHYR=$(whereis -b Xephyr | cut -f2 -d' ')
xinit ./xinitrc -- \
    "$XEPHYR" \
        :100 \
        -ac \
        -screen 800x600 \
        -host-cursor
