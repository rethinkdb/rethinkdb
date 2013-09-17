#!/bin/sh

set -e
set -u

main () {
    init
    log "Preparing to install the Python driver for RethinkDB"
    log "See http://www.rethinkdb.com/docs/install-drivers/python/ for more information"
    log "Logging to $log_file"

    each "apt brew portage source pip" list_methods
    each "apt brew portage source pip" info
    return

    log "Inspecting the environment..."
    inspect_environment
    log "Checking for dependencies..."
    check dependencies
    if any missing; then
        if can install missing; then
            log
            log "The following will be installed:"
            info missing
            if user id != 0 && any missing requires sudo; then
                log "Superuser access is required to complete the installation."
                log "Re-run this script using sudo to complete the installation."
                exit 1
            fi
            confirm "Do you want to proceed with the installation? [yn] "
            install missing
            log "Installation complete!"
        else
            log
            log "The following dependencies are missing and cannot be installed automatically:"
            for dep in $(missing); do
                if cannot install $dep; then
                    info $dep
                fi
            done
            log "Install the missing dependencies and run this script again."
        fi
    fi
    cleanup
}

init () {
    dependencies=TODO
    missing=
    package_manager=
    tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/rethinkdb-python-install.XXXXXXXX")
    log_file=$tmp_dir/install.log
    this=
    stack=root
}

cleanup () {
    test -n "$tmp_dir" && rm -rf "$tmp_dir"
}

log () {
    echo "$*" >&3
    test -n "$log_file" && echo "$*" >> "$log_file"
}

inspect_environment () {
    os=$(uname -s || echo Unknown) 2>/dev/null
    log "Operating system: $os"
    case "$os" in
        Linux)
            distro=$(detect_distro)
            log "Distribution: $distro"
            case "$distro" in
                Ubuntu) package_manager=apt ;;
                Debian*) package_manager=apt ;;
                Gentoo) package_manager=portage ;;
                *)
                    if apt-get -h >/dev/null 2>&1; then
                        package_manager=apt
                    fi
            esac
            ;;
        Darwin)
            if has_bin brew; then
                package_manager=brew
            fi
            ;;
    esac
    log "Package Manager: $(info pkg)"
}

pkg () { either "$package_manager pip source" "$@" ; }

has_bin () {
    which "$1" 2>/dev/null
}

detect_distro () {
    {   lsb_release -si ||
        { test -e /etc/gentoo-release && echo Gentoo ; } ||
        { grep -q 'ID_LIKE=debian' /etc/os-release && echo Debian-based ; } ||
        echo Other
    } 2>/dev/null
}

package_manager () {
    $class package_manager

    method info "$@" || {
        error "$this has no info method"
    }

    nomethod "$@"
}

apt () {
    $class apt

    method info "$@" || {
        echo "APT package manager"
    }

    inherit package_manager "$@"
}

portage () {
    $class portage
    inherit package_manager
}

brew () {
    $class brew
    inherit package_manager
}

source () {
    $class source
    inherit package_manager
}

pip () {
    $class pip
    inherit package_manager
}

dependencies () { each "$dependencies" "$@" ; }

missing () { each "$missing" "$@" ; }

any () { obj=$1; shift; $obj any "$@" ; }

check () { $1 check ; }

can () { $2 can "$1" ; }

cannot () { not $2 can "$1" ; }

not () { ! "$@" ; }

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
        list_methods) echo "$name" ;;
        $name) return 1 ;;
        *) stack="$stack $name"; return ;;
    esac
}

nomethod () {
    error "No such method $1 for $this"
}

error () {
    last_error"$*"
    return 1
}

class='eval local this; local stack=$stack; _class'
_class () {
    if [ "${this#inherit *}" = "" ]; then
        set -- $this
        this=$2
    else
        this=$*
        stack="$stack | $this"
    fi
}

inherit () {
    this="inherit $this"
    "$@"
}

either () {
    local list=$1
    shift 1

    method "" "$@" || {
        echo "$list"
        return
    }

    method "*" "$@" || {
        shift
        for object in $list; do
            $object "$@" && return || true
        done
        return 1
    }
}

main "$@" 3>&1 || {
    log "ERROR: $last_error"
    log "Aborting installation"
    exit 1
}
