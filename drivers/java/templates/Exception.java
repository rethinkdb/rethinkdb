package com.rethinkdb;

public class ${camel(classname)} extends ${camel(superclass)} {
    public ${camel(classname)}() {}

    public ${camel(classname)}(String message) {
        super(message);
    }

    public ${camel(classname)}(String format, Object... args) {
        super(String.format(format, args));
    }

    public ${camel(classname)}(String message, Throwable cause) {
        super(message, cause);
    }

    public ${camel(classname)}(Throwable cause) {
        super(cause);
    }
}
