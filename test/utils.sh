#!/bin/bash

# GENERATE_POG <component>
# Generates propperly indented POG files with required attributes
#
# DISPLAY_CONFIGURATION
# Prints the script configuration to the terminal
#
# Set up the following variables
# - BXML: Path to bxml binary (Atelier B program)
# - POG: Path to pog binary (Atelier B program)
# - POG_PARAMETER: Path to parameter file (Atelier B configuration file)
# - DETOX: Path to detox utility (<https://gitlab.clearsy.com/AtelierB/butils>)
BXML=$HOME/bin/bxml
POG=$HOME/bin/pog
DETOX=$HOME/bin/detox


DISPLAY_CONFIGURATION() {
    echo "BXML=$BXML"
    echo "POG=$POG"
    echo "DETOX=$DETOX"
}

GENERATE_POG() {
    COMPONENT=$1

    [ -n "$BXML" ] && "$BXML" -0 -a -i 2 -O . "$COMPONENT.mch"
    [ -n "$POG" ] && "$POG" -0 "$COMPONENT.bxml" -z pog
    [ -n "$DETOX" ] && "$DETOX" --tag "$COMPONENT.pog"
    rm -f "$COMPONENT.bxml" "$COMPONENT.pos"
}

export -f GENERATE_POG
