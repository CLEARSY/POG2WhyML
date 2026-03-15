#!/bin/bash

# Iterate over smt2 files in reference to check that they are valid.

dolmencommand="$HOME/bin/dolmen"
why3command="/usr/bin/why3"

references=$(find -name reference -type d)

why_files_OK=0
tptp_files_OK=0
files_tot=0

echo "Checking SMT with dolmen"
for dir in $references; do
    files=$(find $dir -name "*.why")
    for file in $files; do
        prefix=$(basename -s ".why" $file)
        wdir=$(dirname $file)
        whyfile=$wdir/${prefix}.why
        tptpfile=$wdir/${prefix}.p
        $why3command prove -L ../etc -D zenon_modulo $whyfile > $tptpfile
	    if [[ $? -ne 0 ]]; then
            echo "    NOK: $whyfile may not be correct."
        else
            echo "     OK: $whyfile is correct."
            ((why_files_OK=why_files_OK+1))
            $dolmencommand -i tptp $tptpfile > /dev/null
    	    if [[ $? -ne 0 ]]; then
                echo "    NOK: $tptpfile may not be correct."
            else
                echo "     OK: $tptpfile is correct."
                ((tptp_files_OK=tptp_files_OK+1))
            fi
        fi
        ((files_tot=files_tot+1))
    done
done

echo ""
echo "Summary :"
echo "  Why3:  $why_files_OK/$files_tot"
echo "  TPTP: $tptp_files_OK/$tptp_files_tot"
