class ExecutionError(StandardError):
    def __init__(self, message, ast_path, query):
        self.message = message
        self.ast_path = ast_path
        self.query = query

    def __str__(self):
        return "%s %s %s" % (self.message, self.ast_path, self.query)

class BadQueryError(ExecutionError):
    pass
