#!/bin/csh -f

foreach aux ( $* )
    echo Munging $aux
    /bin/mv $aux $aux.bak
    cat $aux.bak | perl jwebfrob.pl > $aux
    /bin/cp $aux $aux.munged
end
