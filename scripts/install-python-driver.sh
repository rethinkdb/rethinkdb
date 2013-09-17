#!/bin/sh

set -e
set -u

main () {
    echo "Preparing to install the Python driver for RethinkDB"
    echo "See http://www.rethinkdb.com/docs/install-drivers/python/ for more information"
    echo "Detecting the environment..."
    detect environment
    echo "Checking for dependencies..."
    check dependencies
    if any missing; then
        if can install missing; then
            echo
            echo "The following will be installed:"
            info missing
            if user id != 0 && any missing requires sudo; then
                echo "Superuser access is required to complete the installation."
                echo "Re-run this script using sudo to complete the installation."
                exit 1
            fi
            confirm "Do you want to proceed with the installation? [yn] "
            install missing
            echo "Installation complete!"
        else
            echo
            echo "The following dependencies are missing and cannot be installed automatically:"
            for dep in $(missing); do
                if cannot install $dep; then
                    info $dep
                fi
            done
            echo "Install the missing dependencies and run this script again."
        fi
    fi
}

dependencies=TODO

missing=

package_manager=

# check, can install, info

environment () {
    local self=environment

    method detect "$@" || {
        os=$(uname -s || echo Unknown) 2>/dev/null
        echo "Operating system: $os"
        distro=$(detect_distro)
    }
}

detect_distro () {
    {   lsb_release -si ||
        { test -e /etc/gentoo-release && echo Gentoo ; } ||
        echo Other
    } 2>/dev/null
}

dependencies () { each "$dependencies" "$@" }

missing () { each "$missing" "$@" }

environment () { TODO }

any () { obj=$1; shift; $obj any "$@" }

check () { $1 check }

detect () { $1 detect }

can () { $2 can "$1" }

cannot () { not $2 can "$1" }

not () { ! "$@" }

each () {
    local list=$1
    shift 1

    method "" "$@" || {
        echo "$list"
        return
    }

    method any "$@" || {
        for object in $list; do
            $object "$@" && return 0
        done
        return 1
    }

    method "*" "$@" || {
        shift
        for object in $list; do
            $object "$@"
        done
        return
    }
}

method () {
    local name=$1
    case "${2:-}" in
        has_method) return ;;
        list_methods) echo "$name" ;;
        $name) return 1 ;;
        *) return ;;
    esac
}
