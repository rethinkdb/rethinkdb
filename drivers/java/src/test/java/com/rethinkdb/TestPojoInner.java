package com.rethinkdb;

/**
 * Has only public parametrized constructor and no public parameterless constructor
 */
public class TestPojoInner {
    private Long longProperty;
    private Boolean booleanProperty;
    private TestPojoEnum enumProperty;

    public TestPojoInner() {
    }

    public TestPojoInner(Long longProperty, Boolean booleanProperty, TestPojoEnum enumProperty) {
        this.longProperty = longProperty;
        this.booleanProperty = booleanProperty;
        this.enumProperty = enumProperty;
    }

    public Long getLongProperty() { return this.longProperty; }
    public Boolean getBooleanProperty() { return this.booleanProperty; }
    public TestPojoEnum getEnumProperty() { return this.enumProperty; }

    public void setLongProperty(Long longProperty) { this.longProperty = longProperty; }
    public void setBooleanProperty(Boolean booleanProperty) { this.booleanProperty = booleanProperty; }
    public void setEnumProperty(TestPojoEnum enumProperty) {this.enumProperty = enumProperty; }
}
