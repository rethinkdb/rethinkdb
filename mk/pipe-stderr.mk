
# Used to filter out some error messages. For example:
# $(call pipe-stderr, ls -l $(LIST), grep -v 'not found')
# 
# Use fd 9 because make uses 3 and 4 by default
pipe-stderr = ( $1 2>&1 1>&9 9>&- | ($2 || true) 1>&2 9>&-) 9>&1