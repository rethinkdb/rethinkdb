public class TestPojo {
    private String stringProperty;
    private TestPojoInner pojoProperty;

    public TestPojo() {}

    TestPojo(String stringProperty, TestPojoInner pojoProperty) {
        this.stringProperty = stringProperty;
        this.pojoProperty = pojoProperty;
    }

    public String getStringProperty() { return this.stringProperty; }
    public TestPojoInner getPojoProperty() { return this.pojoProperty; }

    public void setStringProperty(String stringProperty) { this.stringProperty = stringProperty; }
    public void setPojoProperty(TestPojoInner pojoProperty) { this.pojoProperty = pojoProperty; }
}
