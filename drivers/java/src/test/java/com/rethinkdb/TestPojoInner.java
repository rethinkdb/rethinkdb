package com.rethinkdb;

/**
 * Has only public parametrized constructor and no public parameterless constructor
 */
public class TestPojoInner {
    private Long longProperty;
    private Boolean booleanProperty;

    public TestPojoInner() {
    }

    public TestPojoInner(Long longProperty, Boolean booleanProperty) {
        this.longProperty = longProperty;
        this.booleanProperty = booleanProperty;
    }

    public Long getLongProperty() { return this.longProperty; }
    public Boolean getBooleanProperty() { return this.booleanProperty; }

    public void setLongProperty(Long longProperty) { this.longProperty = longProperty; }
    public void setBooleanProperty(Boolean booleanProperty) { this.booleanProperty = booleanProperty; }
}
