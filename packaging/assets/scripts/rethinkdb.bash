_rethinkdb_value_in_array () {
    local item value=$1
    shift
    for item; do
        [[ "$item" == "$value" ]] && return 0
    done
    return 1
}

_complete_rethinkdb_admin() {
    local partiality="partial"
    local cur="${COMP_WORDS[COMP_CWORD]}"
    if [ "$cur" == "" ]; then
        partiality="full"
    fi

    # To get completions, we will call "rethinkdb admin complete", which will print out the possible completions
    if [ -x ${COMP_WORDS[0]} ]; then
        # provide the --join parameter (if specified) and filter out other parameters
        local join_params=( )
        local passthrough_params=( )
        local skip=0
        for index in ${!COMP_WORDS[*]}; do
            if [ $index -ge 2 ]; then
                if [ $skip -ne 0 ]; then
                    skip=$(( $skip - 1 ))
                elif [ "${COMP_WORDS[$index]}" == "--join" ] || [ "${COMP_WORDS[$index]}" == "-j" ]; then
                    # For some reason bash completion will break up the "host:port" parameter into three: "host" ":" "port"
                    local host_index=$(( $index + 1 ))
                    local port_index=$(( $index + 3 ))
                    # TODO: make this checking more robust in case of an invalid statement
                    if [ "$port_index" -lt "${#COMP_WORDS[*]}" ]; then
                        join_params=("--join" "${COMP_WORDS[$host_index]}:${COMP_WORDS[$port_index]}")
                    fi
                    skip=3
                elif [ "${COMP_WORDS[$index]}" == "--client-port" ]; then # TODO: this is a debug only option...
                    skip=1
                else
                    passthrough_params=( ${passthrough_params[@]} ${COMP_WORDS[$index]} )
                fi
            fi
        done
        local complete_params=("${COMP_WORDS[0]}" "admin" "complete" "${join_params[@]}" "$partiality" "${passthrough_params[@]}")
        local result=$("${complete_params[@]}" 2>/dev/null)
        COMPREPLY=( $( compgen -W "$result" -- "$cur" ) )
    else
        COMPREPLY=( )
    fi
}

_complete_rethinkdb() {
    local io_backend=("--io-backend")
    local io_backends=("native" "pool")
    local format_args=("--format")
    local formats=("csv" "json")
    local commands=("create" "help" "serve" "admin" "proxy" "import")
    local file_args=("--input-file" "--pid-file" "-f" "--file")
    local directory_args=("-d" "--directory" "-l" "--log-file")
    local numb_args=("-c" "--cores" "--client-port" "--cluster-port" "--driver-port" "-o" "--port-offset" "--http-port" "--clients")
    local help_tokens=("create" "serve" "admin" "proxy" "export" "import" "dump" "restore")
    local create_tokens=("-d" "--directory" "-n" "--server-name" "--io-backend")
    local serve_tokens=("-d" "--directory" "--cluster-port" "--driver-port" "-o" "--port-offset" "-j" "--join" "--http-port" "-c" "--cores" "--pid-file" "--io-backend")
    local proxy_tokens=("--log-file" "--cluster-port" "--driver-port" "-o" "--port-offset" "-j" "--join" "--http-port" "--pid-file" "--io-backend")
    local export_tokens=("-c" "--connect" "-a" "--auth" "-d" "--directory" "-e" "--export" "--format" "--fields")
    local import_tokens=("-c" "--connect" "-a" "--auth" "-d" "--directory" "-i" "--import" "-f" "--file" "--format" "--table" "--pkey" "--clients" "--force")
    local dump_tokens=("-c" "--connect" "-a" "--auth" "-e" "--export" "-f" "--file")
    local restore_tokens=("-c" "--connect" "-a" "--auth" "-i" "--import" "--clients" "--force")

    local cur=${COMP_WORDS[COMP_CWORD]}
    local use=""

    if [ "$COMP_CWORD" -eq 1 ]; then
        #for some reason this only works if I load it into use first
        use="${commands[@]}"
        COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
        return
    else
        local command="${COMP_WORDS[1]}"
        local prev="${COMP_WORDS[COMP_CWORD-1]}"

        if _rethinkdb_value_in_array "$prev" "${directory_args[@]}"; then
            COMPREPLY=( $( compgen -d -- "$cur") )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${file_args[@]}"; then
            COMPREPLY=( $( compgen -A file -- "$cur" ) )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${numb_args[@]}"; then
            COMPREPLY=( )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${io_backend[@]}"; then
            use="${io_backends[@]}"
            COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${format_args[@]}"; then
            use="${formats[@]}"
            COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
            return
        fi

        case "$command" in
            help) use="${help_tokens[@]}" ;;
            create) use="${create_tokens[@]}" ;;
            serve) use="${serve_tokens[@]}" ;;
            proxy) use="${proxy_tokens[@]}" ;;
            import) use="${import_tokens[@]}" ;;
            admin) _complete_rethinkdb_admin $command
                return ;;
        esac
        COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
    fi
}

complete -F _complete_rethinkdb SERVER_EXEC_NAME
complete -F _complete_rethinkdb SERVER_EXEC_NAME_VERSIONED
