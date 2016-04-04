package com.rethinkdb;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

/**
 * Has only public parametrized constructor and no public parameterless constructor
 */
public class TestPojoInner {
    public static class TestPojoInnerInner {
        private String stringProperty;

        public TestPojoInnerInner() {
        }

        public TestPojoInnerInner(String stringProperty) {
            this.stringProperty = stringProperty;
        }

        public String getStringProperty() {
            return stringProperty;
        }

        public void setStringProperty(String stringProperty) {
            this.stringProperty = stringProperty;
        }
    }

    private Long longProperty;
    private Boolean booleanProperty;

    public ArrayList<TestPojoInnerInner> getInnerInnerPojoArrayListProperty() {
        return innerInnerPojoArrayListProperty;
    }

    public void setInnerInnerPojoArrayListProperty(ArrayList<TestPojoInnerInner> innerInnerPojoArrayListProperty) {
        this.innerInnerPojoArrayListProperty = innerInnerPojoArrayListProperty;
    }

    private ArrayList<TestPojoInnerInner> innerInnerPojoArrayListProperty;

    public TestPojoInner() {
    }

    public TestPojoInner(Long longProperty, Boolean booleanProperty) {
        this.longProperty = longProperty;
        this.booleanProperty = booleanProperty;
        this.innerInnerPojoArrayListProperty = new ArrayList<>();
        this.innerInnerPojoArrayListProperty.add(new TestPojoInnerInner("innerInner111" + ";" + longProperty + ";" + booleanProperty));
        this.innerInnerPojoArrayListProperty.add(new TestPojoInnerInner("innerInner112" + ";" + longProperty + ";" + booleanProperty));
    }

    public Long getLongProperty() { return this.longProperty; }
    public Boolean getBooleanProperty() { return this.booleanProperty; }

    public void setLongProperty(Long longProperty) { this.longProperty = longProperty; }
    public void setBooleanProperty(Boolean booleanProperty) { this.booleanProperty = booleanProperty; }
}
