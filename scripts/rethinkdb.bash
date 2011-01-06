_complete_rethinkdb() {
    commands='extract create help fsck serve'
    extract_tokens="-f --file --force-block-size --force-extent-size --force-slice-count -l --log-file -o --output-file"
    create_tokens="-f --file -s --slices --block-size --extent-size -l --log-file --force"
    help_tokens="$commands"
    fsck_tokens="-f --file -l --log-file"
    serve_tokens="-f --file -c --cores -m --max-cache-size -p --port --wait-for-flush --flush-timer --flush-threshold --gc-range --active-data-extents -v --verbose -l --log-file"
    cur=${COMP_WORDS[COMP_CWORD]}
    if [ "$COMP_CWORD" -eq 1 ]; then
        COMPREPLY=( $( compgen -W "$commands" -- "$cur" ) )
        return
    else
        command="${COMP_WORDS[1]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
        if [ "$prev" = "-f" ] || [ "$prev" = "--file" ] || [ "$prev" = "-l" ] || [ "$prev" = "--log-file" ] || (([ "$prev" = "-o" ] || [ "$prev" = "--output-file" ]) && [ "$command" = "extract" ]); then
            COMPREPLY=( $( compgen -A file -- "$cur" ) )
            return
        else
            case "$command" in
                extract) use="$extract_tokens";; 
                create) use="$create_tokens" ;;
                help) use="$help_tokens" ;;
                fsck) use="$fsck_tokens" ;;
                serve) use="$serve_tokens" ;;
            esac
            COMPREPLY=( $( compgen -W "$use" -- "$cur" ) )
        fi
    fi
}

complete -o default -F _complete_rethinkdb rethinkdb
