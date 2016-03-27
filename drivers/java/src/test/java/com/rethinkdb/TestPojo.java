package com.rethinkdb;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.time.OffsetDateTime;
import java.time.ZonedDateTime;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * Has both public parameterless constructor and public parametrized constructor
 */
public class TestPojo {

    public enum PojoEnum {
        AAA("aaa"),
        xxx("xxx"),
        XXX("XXX"),
        ;

        private String value;

        PojoEnum(String value) {
            this.value = value;
        }

        @Override
        public String toString() {
            return value;
        }
    }

    private String stringProperty;
    private TestPojoInner pojoProperty;
    private PojoEnum enumProperty;
    private PojoEnum enumInnerLowerCaseProperty;
    private PojoEnum enumInnerUpperCaseProperty;
    private OffsetDateTime offsetDateTimeProperty;
    private LocalDateTime localDateTimeProperty;
    private ZonedDateTime zonedDateTimeProperty;
    private LocalDate localDateProperty;
    private LocalTime localTimeProperty;
    private Date dateProperty;
    private Double doubleProperty;
    private double primitiveDoubleProperty;
    private Float floatProperty;
    private float primitiveFloatProperty;
    private Integer integerProperty;
    private int primitiveIntegerProperty;
    private Long longProperty;
    private long primitiveLongProperty;
    private Short shortProperty;
    private short primitiveShortProperty;
    private Byte byteProperty;
    private byte primitiveByteProperty;
    private Boolean booleanProperty;
    private boolean primitiveBooleanProperty;
    private BigDecimal bigDecimalProperty;
    private BigInteger bigIntegerProperty;

    private List<TestPojoInner> pojoListProperty;

    private TestPojoInner[] pojoArrayProperty;

    private int[] primitiveIntegerArrayProperty;

    private static final OffsetDateTime sample_offsetDateTimeProperty = OffsetDateTime.now();
    private static final LocalDateTime sample_localDateTimeProperty = LocalDateTime.now();
    private static final ZonedDateTime sample_zonedDateTimeProperty = ZonedDateTime.now();
    private static final LocalDate sample_localDateProperty = LocalDate.now();
    private static final LocalTime sample_localTimeProperty = LocalTime.now();
    private static final Date sample_dateProperty = new Date();

    public TestPojo() {}

    public TestPojo(String stringProperty, TestPojoInner pojoProperty) {
        this.stringProperty = stringProperty;
        this.pojoProperty = pojoProperty;

        enumProperty = PojoEnum.AAA;
        enumInnerLowerCaseProperty = PojoEnum.xxx;
        enumInnerUpperCaseProperty = PojoEnum.XXX;
        offsetDateTimeProperty = sample_offsetDateTimeProperty;
        localDateTimeProperty = sample_localDateTimeProperty;
        zonedDateTimeProperty = sample_zonedDateTimeProperty;
        localDateProperty = sample_localDateProperty;
        localTimeProperty = sample_localTimeProperty;
        dateProperty = sample_dateProperty;
        doubleProperty = Double.MAX_VALUE;
        primitiveDoubleProperty = Double.MAX_VALUE;
        floatProperty = Float.MAX_VALUE;
        primitiveFloatProperty = Float.MAX_VALUE;
        integerProperty = Integer.MAX_VALUE;
        primitiveIntegerProperty = Integer.MAX_VALUE;
        longProperty = Long.MAX_VALUE;
        primitiveLongProperty = Long.MAX_VALUE;
        shortProperty = Short.MAX_VALUE;
        primitiveShortProperty = Short.MAX_VALUE;
        byteProperty = Byte.MAX_VALUE;
        primitiveByteProperty = Byte.MAX_VALUE;
        booleanProperty = true;
        primitiveBooleanProperty = true;
        bigDecimalProperty = new BigDecimal("123456789012345678901234567");
        bigIntegerProperty = new BigInteger("123456789012345");

        pojoListProperty = Arrays.asList(new TestPojoInner(111L, true), new TestPojoInner(222L, false));
        pojoArrayProperty = new TestPojoInner[] {new TestPojoInner(111L, true), new TestPojoInner(222L, false)};
//        primitiveIntegerArrayProperty = new int[] {991, 992};
    }

    public String getStringProperty() { return this.stringProperty; }
    public TestPojoInner getPojoProperty() { return this.pojoProperty; }

    public void setStringProperty(String stringProperty) { this.stringProperty = stringProperty; }
    public void setPojoProperty(TestPojoInner pojoProperty) { this.pojoProperty = pojoProperty; }

    public PojoEnum getEnumProperty() {
        return enumProperty;
    }

    public void setEnumProperty(PojoEnum enumProperty) {
        this.enumProperty = enumProperty;
    }

    public PojoEnum getEnumInnerLowerCaseProperty() {
        return enumInnerLowerCaseProperty;
    }

    public void setEnumInnerLowerCaseProperty(PojoEnum enumInnerLowerCaseProperty) {
        this.enumInnerLowerCaseProperty = enumInnerLowerCaseProperty;
    }

    public PojoEnum getEnumInnerUpperCaseProperty() {
        return enumInnerUpperCaseProperty;
    }

    public void setEnumInnerUpperCaseProperty(PojoEnum enumInnerUpperCaseProperty) {
        this.enumInnerUpperCaseProperty = enumInnerUpperCaseProperty;
    }

    public OffsetDateTime getOffsetDateTimeProperty() {
        return offsetDateTimeProperty;
    }

    public void setOffsetDateTimeProperty(OffsetDateTime offsetDateTimeProperty) {
        this.offsetDateTimeProperty = offsetDateTimeProperty;
    }

    public LocalDateTime getLocalDateTimeProperty() {
        return localDateTimeProperty;
    }

    public void setLocalDateTimeProperty(LocalDateTime localDateTimeProperty) {
        this.localDateTimeProperty = localDateTimeProperty;
    }

    public ZonedDateTime getZonedDateTimeProperty() {
        return zonedDateTimeProperty;
    }

    public void setZonedDateTimeProperty(ZonedDateTime zonedDateTimeProperty) {
        this.zonedDateTimeProperty = zonedDateTimeProperty;
    }

    public LocalDate getLocalDateProperty() {
        return localDateProperty;
    }

    public void setLocalDateProperty(LocalDate localDateProperty) {
        this.localDateProperty = localDateProperty;
    }

    public LocalTime getLocalTimeProperty() {
        return localTimeProperty;
    }

    public void setLocalTimeProperty(LocalTime localTimeProperty) {
        this.localTimeProperty = localTimeProperty;
    }

    public Date getDateProperty() {
        return dateProperty;
    }

    public void setDateProperty(Date dateProperty) {
        this.dateProperty = dateProperty;
    }

    public Double getDoubleProperty() {
        return doubleProperty;
    }

    public void setDoubleProperty(Double doubleProperty) {
        this.doubleProperty = doubleProperty;
    }

    public double getPrimitiveDoubleProperty() {
        return primitiveDoubleProperty;
    }

    public void setPrimitiveDoubleProperty(double primitiveDoubleProperty) {
        this.primitiveDoubleProperty = primitiveDoubleProperty;
    }

    public Float getFloatProperty() {
        return floatProperty;
    }

    public void setFloatProperty(Float floatProperty) {
        this.floatProperty = floatProperty;
    }

    public float getPrimitiveFloatProperty() {
        return primitiveFloatProperty;
    }

    public void setPrimitiveFloatProperty(float primitiveFloatProperty) {
        this.primitiveFloatProperty = primitiveFloatProperty;
    }

    public Integer getIntegerProperty() {
        return integerProperty;
    }

    public void setIntegerProperty(Integer integerProperty) {
        this.integerProperty = integerProperty;
    }

    public int getPrimitiveIntegerProperty() {
        return primitiveIntegerProperty;
    }

    public void setPrimitiveIntegerProperty(int primitiveIntegerProperty) {
        this.primitiveIntegerProperty = primitiveIntegerProperty;
    }

    public Long getLongProperty() {
        return longProperty;
    }

    public void setLongProperty(Long longProperty) {
        this.longProperty = longProperty;
    }

    public long getPrimitiveLongProperty() {
        return primitiveLongProperty;
    }

    public void setPrimitiveLongProperty(long primitiveLongProperty) {
        this.primitiveLongProperty = primitiveLongProperty;
    }

    public Short getShortProperty() {
        return shortProperty;
    }

    public void setShortProperty(Short shortProperty) {
        this.shortProperty = shortProperty;
    }

    public short getPrimitiveShortProperty() {
        return primitiveShortProperty;
    }

    public void setPrimitiveShortProperty(short primitiveShortProperty) {
        this.primitiveShortProperty = primitiveShortProperty;
    }

    public Byte getByteProperty() {
        return byteProperty;
    }

    public void setByteProperty(Byte byteProperty) {
        this.byteProperty = byteProperty;
    }

    public byte getPrimitiveByteProperty() {
        return primitiveByteProperty;
    }

    public void setPrimitiveByteProperty(byte primitiveByteProperty) {
        this.primitiveByteProperty = primitiveByteProperty;
    }

    public Boolean getBooleanProperty() {
        return booleanProperty;
    }

    public void setBooleanProperty(Boolean booleanProperty) {
        this.booleanProperty = booleanProperty;
    }

    public boolean getPrimitiveBooleanProperty() {
        return primitiveBooleanProperty;
    }

    public void setPrimitiveBooleanProperty(boolean primitiveBooleanProperty) {
        this.primitiveBooleanProperty = primitiveBooleanProperty;
    }

    public BigDecimal getBigDecimalProperty() {
        return bigDecimalProperty;
    }

    public void setBigDecimalProperty(BigDecimal bigDecimalProperty) {
        this.bigDecimalProperty = bigDecimalProperty;
    }

    public BigInteger getBigIntegerProperty() {
        return bigIntegerProperty;
    }

    public void setBigIntegerProperty(BigInteger bigIntegerProperty) {
        this.bigIntegerProperty = bigIntegerProperty;
    }


    public List<TestPojoInner> getPojoListProperty() {
        return pojoListProperty;
    }

    public void setPojoListProperty(List<TestPojoInner> pojoListProperty) {
        this.pojoListProperty = pojoListProperty;
    }

    public TestPojoInner[] getPojoArrayProperty() {
        return pojoArrayProperty;
    }

    public void setPojoArrayProperty(TestPojoInner[] pojoArrayProperty) {
        this.pojoArrayProperty = pojoArrayProperty;
    }

    public int[] getPrimitiveIntegerArrayProperty() {
        return primitiveIntegerArrayProperty;
    }

    public void setPrimitiveIntegerArrayProperty(int[] primitiveIntegerArrayProperty) {
        this.primitiveIntegerArrayProperty = primitiveIntegerArrayProperty;
    }
}
