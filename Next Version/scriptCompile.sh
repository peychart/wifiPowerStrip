#!/bin/sh
#Usage: ./$0 <script.esp >scriptCode
sed -e 's;//.*$;;' -e 's;\(.*\);\L\1;g' -e 's;if[ \t];;' -e 's;[ \t]then[ \t];?;' -e 's;[ \t];;g' -e 's;+; ;g' -e 's;&&; ;g' -e 's;setgpio(;S;g' -e 's;gpio(;G;g' -e 's;cleartimer(;T;g' -e 's;timer(;T;g' -e 's;mqttsend(;M;g' -e 's;[()];;g' -e 's;input_pullup;2;g' -e 's;input;1;g' -e 's;output;0;g' -e 's;true;1;g' -e 's;false;0;g' -e 's;\\n$;;' -e 's;";\\";g' -e 's;$;\\;' |awk '!/^\\$/'
