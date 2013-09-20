#!/bin/sh

set -e
set -u

usage () {
  echo "Install the RethinkDB Python driver and its dependencies."
  echo "  install-python-driver.sh [-y|--yes] [-h|--help]"
}

main () {
  init "$@"
  log "Preparing to install the Python driver for RethinkDB."
  log "See http://www.rethinkdb.com/docs/install-drivers/python/ for more information."
  log "Logging to: $log_file"

  log "Inspecting the environment..."
  inspect_environment

  # TODO
  # python version
  # libprotobuf
  # protoc
  # python-dev

  # pyprotobuf

  if silent python -c 'import google.protobuf'; then
    log "The Python protobuf package is missing. It will be installed."
    confirm "Continue?"
    install pyprotobuf
  elif silent python -c 'import google.protobuf.internal.cpp_message'; then
    log "The Python protobuf library does not have the C++ extension. It will be reinstalled."
    confirm "Coninute?"
    install pyprotobuf
  fi

  if ! silent python -c 'import google.protobuf.internal.cpp_message'; then
    log "The installed Python protobuf library still does not have the C++ extension."
    log "Proceeding with the installation anyways."
  fi

  if silent python -c 'import rethinkdb'; then
    log "The Python rethinkdb package is missing. It will be installed."
    confirm "Continue?"
    install pyprotobuf
  elif silent python -c 'import google.protobuf.internal.cpp_message'; then
    log "The Python protobuf library does not have the C++ extension. It will be reinstalled."
    confirm "Coninute?"
    install pyprotobuf
  fi

  if ! protobuf_implementation=$(silent python -c
      'import rethinkdb as r; print(r.protobuf_implementation)');
  then
    log "The Python RethinkDB driver was installed without the faster C++ extension."
    log "See $log_file for more details"
    exit
  fi

  cleanup
}

init () {
  always_yes=false
  pyrethinkdb_version=1.8.0-1

  case "${1:-}" in
    -y|--yes) always_yes=true ;;
    -h|--help) usage; exit ;;
    *) error "Unknown argument $1"
  esac

  tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/rethinkdb-python-install.XXXXXXXX")
  log_file=$tmp_dir/install.log
}

cleanup () {
  test -n "$tmp_dir" && rm -rf "$tmp_dir"
}

log () {
  echo "$*" >&3
  test -n "$log_file" && echo "$*" >> "$log_file"
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
  {   lsb_release -si ||
    { test -e /etc/gentoo-release && echo Gentoo ; } ||
    { grep -q 'ID_LIKE=debian' /etc/os-release && echo Debian-based ; } ||
    echo Other
  } 2>/dev/null
}

silent () {
  echo "$ $*" >> "$log_file"
  "$@" >"$log_file" 2>&1
}

confirm () {
  if $always_yes; then
    log "$* [Yn] Y"
    return
  fi
  local ans
  while true; do
    echo -n "$* [Yn] "
    read ans
    case "$ans" in
      y|Y|) return ;;
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

main "$@" 3>&1 || (
  error "Instalation failed. Logged output to $log_file."
)
