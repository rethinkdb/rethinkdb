package com.rethinkdb;

/**
 * Has only public parametrized constructor and no public parameterless constructor
 */
public enum TestPojoEnum {
    cat("cat"),
    dog("dog"),
    mouse("mouse");

    private String value;

    TestPojoEnum(String value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return value;
    }
}
