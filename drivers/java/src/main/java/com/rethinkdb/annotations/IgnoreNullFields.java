package com.rethinkdb.annotations;

import java.lang.annotation.*;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
@Documented
@Inherited
/**
 * When saving this data to rethink db, skip null properties.
 * To recursively skip null properties of children properties,
 * specify this annotation to the children property's classes.
 */
public @interface IgnoreNullFields {
}