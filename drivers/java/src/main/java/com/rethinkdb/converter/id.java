package com.rethinkdb.converter;

import com.fasterxml.jackson.annotation.JsonInclude;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Created by thejp on 10/25/2016.
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
@Target(ElementType.FIELD)
@Retention(RetentionPolicy.RUNTIME)

/**
 * Use this with the primary key/id of your pojo
 */
public @interface id
{
}
