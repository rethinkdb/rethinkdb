#!/usr/bin/env bash

set -e -u

usage () {
    echo "Usage: $0 [python|javascript|ruby|major|minor|revision]"
    echo "  Increment the revision number of one of the drivers (major.minor.0[-.]revision)"
    echo "  or the major, minor or revision number of rethinkdb itself (major.minor.revision)"
    exit 1
}

main () {
        do_not_commit=false

        if ! git diff-index --quiet --cached HEAD; then
            echo Please commit your changes before running bump_version
            echo The following changes are waiting to be committed:
            git diff --cached --summary
            exit 1
        fi

        cd "`git rev-parse --show-toplevel`"
        rethinkdb_full_version=`scripts/gen-version.sh`
        rethinkdb_version=${rethinkdb_full_version%%-*}
        rethinkdb_top_version=`top "$rethinkdb_version"`
        cd drivers
        
        if [[ $# -eq 0 ]]; then
            usage
        fi

        case "$1" in
            javascript) bump_javascript "$rethinkdb_top_version" ;;
            python)     bump_python "$rethinkdb_top_version" ;;
            ruby)       bump_ruby "$rethinkdb_top_version" ;;
            minor)      bump_minor ;;
            major)      bump_major ;;
            revision)   bump_revision ;;
            *)          usage ;;
        esac
}

# top 1.2.3 -> 1.2
top () {
    major=${1%%.*}
    rest=${1#*.}
    echo -n "$major.${rest%%.*}"
}

# revision 1.2.3 -> 3
revision () {
    echo -n "${1#*.*.}"
}

# driver_revision <driver name> <version>
driver_revision () {
    bottom=${1#*.*.}
    echo -n "${bottom#*[-.]}"
}

bump_javascript () {
    current_version=`perl -ne "/version: '(.*?)'/ && print $""1" javascript/package.json`
    new_version=`bump javascript "$current_version" "$1"`
    perl -i -pe "s/version: '.*?'/version: '$new_version'/" javascript/package.json
    driver_commit javascript "$new_version" javascript/package.json
}

bump_python () {
    current_version=`perl -ne '/version="(.*?)"/ && print $1' python/setup.py`
    new_version=`bump python "$current_version" "$1"`
    perl -i -pe 's/version=".*?"/version="'"$new_version"'"/' python/setup.py
    driver_commit python "$new_version" python/setup.py
}

bump_ruby () {
    current_version=`perl -ne "/version.*?= '(.*?)'/ && print "'$'"1" ruby/rethinkdb.gemspec`
    new_version=`bump ruby "$current_version" "$1"`
    perl -i -pe 's/(version.*?= '"'"').*?('"'"')/$1 . "'"$new_version"'" . $2/e' ruby/rethinkdb.gemspec
    driver_commit ruby "$new_version" ruby/rethinkdb.gemspec
}

# driver_commit <driver name> <new version> <file to comit>
driver_commit () {
    if ! $do_not_commit; then
        git add "$3"
        git commit -m "Update $1 driver version to $2"
        git tag -a -m "$1 driver version $2" "$1-v$2"
    
        success
    fi
}

rev_sep () {
    case "$1" in
        javascript) echo -n - ;;
            python) echo -n - ;;
              ruby) echo -n . ;;
    esac
}

# bump <driver name> <driver version> <rethinkdb top version>
bump () {
    echo Currrent $1 version: "'$2'" >&3
    current_top_version=`top "$2"`
    current_revision=`driver_revision "$2"`
    if [[ "$current_top_version" == "$3" ]]; then
        new_version=$current_top_version.0`rev_sep $1`$((current_revision + 1))
    else
        new_version=$3.0`rev_sep $1`0
    fi
    echo New $1 version: "'$new_version'" >&3
    echo -n "$new_version"
}

bump_major () {
    echo Current rethinkdb version: $rethinkdb_version
    new_version=$((${rethinkdb_version%%.*} + 1)).0.0
    echo New rethinkdb version: $new_version
    bump_rethinkdb "$new_version" bump_drivers
}

bump_minor () {
    echo Current rethinkdb version: $rethinkdb_version
    major=${rethinkdb_top_version%.*}
    minor=${rethinkdb_top_version#*.}
    new_version=$major.$((minor+1)).0
    echo New rethinkdb version: $new_version
    bump_rethinkdb "$new_version" bump_drivers
}

bump_revision () {
    echo Current rethinkdb version: $rethinkdb_version
    revision=`revision "$rethinkdb_version"`
    new_version=$rethinkdb_top_version.$((revision + 1))
    echo New rethinkdb version: $new_version
    bump_rethinkdb "$new_version" do_not_bump_drivers
}

bump_rethinkdb () {
    if git show-ref "v$1"; then
        echo "Error: git tag v$1 already exists"
        exit 1
    fi
    
    if test -t 0; then # stdin is a terminal
        cat > ../NOTES.new <<EOF
# Release $1 (RELEASE NAME) #

## Changes ##

* ENTER THE RELEASE NOTES HERE
EOF
        
        while true; do
            ${EDITOR:-vim} ../NOTES.new
            
            if grep -q RELEASE ../NOTES; then
                echo -n "The release notes are not complete. Edit or abort? [eA] "
                read ans
                if [[ "$ans" != "e" ]]; then
                    echo Aborting.
                    exit 1
                fi
            else
                cat ../NOTES.new
                echo
                echo -n "Proceed with the version bump? [yN]"
                read ans
                if [[ "$ans" != "y" ]]; then
                    echo Aborting.
                    exit 1
                else
                    break
                fi
            fi
        done
    else
        # stdin is not a terminal.
        cat > ../NOTES.new
    fi

    cat >> ../NOTES.new <<EOF

---

EOF
    cat ../NOTES >> ../NOTES.new
    mv -f ../NOTES.new ../NOTES

    git add ../NOTES

    if [[ "$2" == bump_drivers ]]; then
        do_not_commit=true
        rethindb_top_version=`top "$1"`
        bump_javascript "$rethindb_top_version"
        bump_python "$rethindb_top_version"
        bump_ruby "$rethindb_top_version"
        git add -u .
    fi

    git commit -m "Prepare for v$1 release"
    git tag -a -m "v$1 release" "v$1"

    success
}

success () {
    echo "The new version is tagged and commited."
    echo "You must manually run \`git push && git push --tags' to share these modifications"
}

main "$@" 3>&1
