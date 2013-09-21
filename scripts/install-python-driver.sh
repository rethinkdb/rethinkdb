#!/bin/sh

set -e
set -u

usage () {
  echo "Install the RethinkDB Python driver and its dependencies."
  echo "  install-python-driver.sh [-y|--yes] [-h|--help] [-v|--verbose] [--generate-install-command <url>]"
}

main () {
  init "$@"
  log "Preparing to install the Python driver for RethinkDB."
  log "See http://www.rethinkdb.com/docs/install-drivers/python/ for more information."
  log "Logging to: $log_file"

  log "Inspecting the environment..."
  inspect_environment

  # TODO
  # check that python is 2.6, 2.7 or > 3.3
  # if it is > 3.3, install the python 3 compatible protobuf library
  # install libprotobuf
  # install protobuf_compiler
  # make sure libprootbuf and protobuf compiler have the same version
  # on debian/ubuntu, the python${VERSION}-dev package must be installed
  # gcc, make, etc.. need to be installed (aka build-essentials)

  python_rethinkdb setup

  if ! python_rethinkdb has_cpp_extension; then
    log "The Python RethinkDB driver was installed without the fast C++ backend."
    log "See $log_file for more details"
  else
    log "The Python RethinkDB driver was installed successfully along with the the fast C++ backend."
    cleanup
  fi
}

init () {
  always_yes=false
  pyrethinkdb_version=1.8.0-1
  verbose=false

  while [ $# -gt 0 ]; do
    case "${1:-}" in
      -y|--yes) always_yes=true; shift ;;
      -h|--help) usage; exit ;;
      -v|--verbose) verbose=true; shift ;;
      --generate-install-command) generate_install_command "$2"; exit ;;
      *) error "Unknown argument $1" ;;
    esac
  done

  tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/rethinkdb-python-install.XXXXXXXX")
  log_file=$tmp_dir/install.log
}

cleanup () {
  test -n "$tmp_dir" && rm -rf "$tmp_dir"
}

log () {
  echo "$*" >&3
  test -n "${log_file:-}" && echo "$*" >> "$log_file"
}

inspect_environment () {
  package_manager=none
  os=$(uname -s || echo Unknown) 2>/dev/null
  log "Operating system: $os"
  case "$os" in
    Linux)
      distro=$(detect_distro)
      log "Distribution: $distro"
      case "$distro" in
        Ubuntu|Debian*) package_manager=apt ;;
        Gentoo) package_manager=portage ;;
      esac
      ;;
    Darwin)
      if silent which brew; then
        package_manager=brew
      fi
      ;;
  esac
  log "Package Manager: $package_manager"
}

detect_distro () {
  { lsb_release -si ||
    { test -e /etc/gentoo-release && echo Gentoo ; } ||
    { grep -q 'ID_LIKE=debian' /etc/os-release && echo Debian-based ; } ||
    echo Other
  } 2>/dev/null
}

silent () {
  if $verbose; then
    log "$ $*"
    "$@" >&3 2>&1
  elif test -n "${log_file:-}"; then
    echo "$ $*" >> "$log_file"
    "$@" >> "$log_file" 2>&1
  else
    "$@" 1>/dev/null 2>&1
  fi
}

confirm () {
  if $always_yes; then
    return
  fi
  local ans
  while true; do
    echo -n "$* [Yn] "
    read ans
    case "$ans" in
      y|Y|"") return ;;
      n|N) return 1 ;;
      *) log "Please answer Y or N" ;;
    esac
  done
  error "No response given"
}

error () {
  log "$*"
  exit 1
}

generate_install_command () {
  local url="$1"
  md5sum=$(which md5 md5sum | head -1)
  md5=$("$md5sum" "$0")
  echo 'curl -o setup.sh '"$url"'&&h=$(which md5 md5sum|head -1)&&m=$(cat setup.sh|$h)&&[ ${m%% *} = '"${md5%% *}"' ]&&./setup.sh||echo "Failed"'
}

python_has_module () {
  silent python -c "import $1"
}

python_has_min_version () {
  silent python <<EOF
from pkg_resources import get_distribution, parse_version
assert get_distribution("$1").parsed_version >= parse_version("$2")
EOF
}

python_protobuf () (

  setup () {
    if ! installed; then
      log "The Python protobuf package is missing. It will be installed."
      confirm "Continue?"
      install
    elif ! has_cpp_extension; then
      log "The Python protobuf library does not have the C++ extension. It will be reinstalled."
      confirm "Continue?"
      install
    fi

    if ! has_cpp_extension; then
      log "The installed Python protobuf library still does not have the C++ extension."
      log "Proceeding with the installation anyways."
    fi
  }

  installed () {
    python_has_module google.protobuf
    python_has_min_version protobuf 2.4.0
  }

  has_cpp_extension () {
    python_has_module google.protobuf.internal.cpp_message
  }

  install () {
    local version="$(protobuf_compiler installed_version)"
    dir=$(download_and_extract "http://protobuf.googlecode.com/files/protobuf-$version.tar.bz2")
    ( cd "$dir/python"
      export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=cpp
      silent python setup.py build
      silent python setup.py install
    )
  }

  "$@"
)

protobuf_compiler () (

  installed_version () {
    local version_string="$(protoc --version)"
    extract_version "$version_string"
  }

  "$@"
)

extract_version () {
  echo "$1" | egrep -o '[0-9]+\.[0-9]+(\.[0-9]+)?' | head -n 1
}

python_rethinkdb () (

  setup () {
    python_protobuf setup
    log "The Python RethinkDB driver will now be installed."
    confirm "Continue?"
    install
  }

  has_cpp_extension () {
    silent python -c 'import rethinkdb as r; assert r.protobuf_implementation == "cpp"'
  }

  install () {
    dir=$(download_and_extract http://download.rethinkdb.com/dist/rethinkdb-1.9.0.tgz)
    ( cd "$dir/drivers/python"
      silent make install || (
        make
        python setup.py install
      )
    )
  }

  "$@"
)

download_and_extract () {
  local geturl=curl
  silent which curl || geturl="wget -O-"
  local filename=${1##*/}
  local dir="$tmp_dir/${filename%.*}"
  mkdir "$dir"
  ( cd "$dir"
    $geturl "$1" > "$filename" 2> "$log_file"
    silent tar xvf "$filename"
    silent rm -vf "$filename"
    set -- *
    if [ $# = 1 ]; then
      echo "$dir/$1"
    else
      error "tarball $filename has more than one top level folder"
    fi
  )
}

trap 'error "Instalation failed. Logged output to $log_file." 3>&1' EXIT
main "$@" 3>&1
trap - EXIT
