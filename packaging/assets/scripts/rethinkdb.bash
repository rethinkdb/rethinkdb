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

    if [ -x ${COMP_WORDS[0]} ]; then
        # TODO: provide the --join parameter (if specified)
        echo words: ${COMP_WORDS[@]}, count: $COMP_CWORD
        echo ${COMP_WORDS[0]} admin complete $partiality ${COMP_WORDS[@]:2}
        local result=`${COMP_WORDS[0]} admin complete $partiality ${COMP_WORDS[@]:2}`
        echo $result
        COMPREPLY=( $( compgen -W "$result" -- "$cur" ) )
    else
        COMPREPLY=( )
    fi
}

_complete_rethinkdb() {
    local io_backend=("--io-backend")
    local io_backends=("native" "pool")
    local commands=("create" "help" "serve" "admin")
    local file_args=("-f" "--file" "-l" "--log-file" "-o" "--output-file" "extract" "--failover-script")
    local directory_args=("-d" "--directory")
    local numb_args=("--force-block-size" "--force-extent-size" "--force-slice-count" "-s" "--slices" "--block-size" "--extent-size" "-c" "--cores" "-m" "--max-cache-size" "-p" "--port" "--flush-timer" "--unsaved-data-limit" "--gc-range" "--active-data-extents")
    local create_tokens=("-f" "--file" "-s" "--slices" "--block-size" "--extent-size" "--diff-log-size" "-l" "--log-file" "--force")
    local help_tokens=("create" "serve" "admin")
    local serve_tokens=("-f" "--file" "-c" "--cores" "-m" "--max-cache-size" "-p" "--port" "--wait-for-flush" "--flush-timer" "--flush-threshold" "--flush-concurrency" "--unsaved-data-limit" "--gc-range" "--active-data-extents" "--io-backend" "--read-ahead" "-v" "--verbose" "-l" "--log-file" "--read-ahead" "--master" "--slave-of" "--failover-script" "--join" "-j" "--directory" "-d")

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
            COMPREPLY= #( $( compgen -W "${arr}" -- "$cur" ) )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${io_backend[@]}"; then
            use="${io_backends[@]}"
            COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
            return
        fi

        case "$command" in
            create) use="${create_tokens[@]}" ;;
            help) use="${help_tokens[@]}" ;;
            admin) _complete_rethinkdb_admin $command
                return ;;
            serve) use="${serve_tokens[@]}" ;;
        esac
        COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
    fi
}

complete -F _complete_rethinkdb SERVER_EXEC_NAME
complete -F _complete_rethinkdb SERVER_EXEC_NAME_VERSIONED
