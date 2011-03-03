_rethinkdb_value_in_array () {
    value="$1"; shift
    arr=("$@") # Ugh
    for VAR in "${arr[@]}"
    do
        if [ "$value" = "$VAR" ]; then
            return 0
        fi
    done
    return 1
}

_complete_rethinkdb() {
    commands=("extract" "create" "help" "fsck" "serve")
    file_args=("-f" "--file" "-l" "--log-file" "-o" "--output-file" "extract")
    numb_args=("--force-block-size" "--force-extent-size" "--force-slice-count" "-s" "--slices" "--block-size" "--extent-size" "-c" "--cores" "-m" "--max-cache-size" "-p" "--port" "--flush-timer" "--unsaved-data-limit" "--gc-range" "--active-data-extents")
    extract_tokens=("-f" "--file" "--force-block-size" "--force-extent-size" "--force-slice-count" "--ignore-diff-log" "-l" "--log-file" "-o" "--output-file")
    create_tokens=("-f" "--file" "-s" "--slices" "--block-size" "--extent-size" "--diff-log-size" "-l" "--log-file" "--force")
    help_tokens=("extract" "create" "fsck" "serve")
    fsck_tokens=("-f" "--file" "-l" "--log-file")
    serve_tokens=("-f" "--file" "-c" "--cores" "-m" "--max-cache-size" "-p" "--port" "--wait-for-flush" "--flush-timer" "--flush-threshold" "--unsaved-data-limit" "--gc-range" "--active-data-extents" "-v" "--verbose" "-l" "--log-file")

    cur=${COMP_WORDS[COMP_CWORD]}

    if [ "$COMP_CWORD" -eq 1 ]; then
        #for some reason this only works if I load it into use first
        use="${commands[@]}"
        COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
        return
    else
        command="${COMP_WORDS[1]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"

        if _rethinkdb_value_in_array "$prev" "${file_args[@]}"; then
            COMPREPLY=( $( compgen -A file -- "$cur" ) )
            return
        fi

        if _rethinkdb_value_in_array "$prev" "${numb_args[@]}"; then
            COMPREPLY= #( $( compgen -W "${arr}" -- "$cur" ) )
            return
        fi

        case "$command" in
            extract) use="${extract_tokens[@]}";; 
            create) use="${create_tokens[@]}" ;;
            help) use="${help_tokens[@]}" ;;
            fsck) use="${fsck_tokens[@]}" ;;
            serve) use="${serve_tokens[@]}" ;;
        esac
        COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
    fi
}

complete -o default -F _complete_rethinkdb rethinkdb
complete -o default -F _complete_rethinkdb rethinkdb-trial
