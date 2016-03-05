package com.rethinkdb;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.time.LocalDate;
import java.time.LocalDateTime;
import java.time.LocalTime;
import java.time.OffsetDateTime;
import java.time.ZonedDateTime;
import java.util.Date;

/**
 * Has both public parameterless constructor and public parametrized constructor
 */
public class TestPojo {

    public enum PojoEnum {
        AAA("aaa"),
        BBB("bbb"),
        CCC("ccc"),
        DDD("ddd")
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
    public static OffsetDateTime sampleOffsetDateTimeProperty = OffsetDateTime.now();
    public static LocalDateTime sampleLocalDateTimeProperty = LocalDateTime.now();
    public static LocalDate sampleLocalDateProperty = LocalDate.now();
    public static LocalTime sampleLocalTimeProperty = LocalTime.now();
    public static ZonedDateTime sampleZonedDateTimeProperty = ZonedDateTime.now();
    public static BigDecimal sampleBigDecimalProperty = new BigDecimal("123456789012345678901234567");
    public static BigInteger sampleBigIntegerProperty = new BigInteger("123456789012345");
    public static Date sampleDateProperty = new Date();

    private String stringProperty;
    private TestPojoInner pojoProperty;
    private PojoEnum enumProperty = PojoEnum.AAA;
    private OffsetDateTime offsetDateTimeProperty = sampleOffsetDateTimeProperty;
    private LocalDateTime localDateTimeProperty = sampleLocalDateTimeProperty;
    private ZonedDateTime zonedDateTimeProperty = sampleZonedDateTimeProperty;
    private LocalDate localDateProperty = sampleLocalDateProperty;
    private LocalTime localTimeProperty = sampleLocalTimeProperty;
    private Date dateProperty = sampleDateProperty;
    private Double doubleProperty = Double.MAX_VALUE;
    private double primitiveDoubleProperty = Double.MAX_VALUE;
    private Float floatProperty = Float.MAX_VALUE;
    private float primitiveFloatProperty = Float.MAX_VALUE;
    private Integer integerProperty = Integer.MAX_VALUE;
    private int primitiveIntegerProperty = Integer.MAX_VALUE;
    private Long longProperty = Long.MAX_VALUE;
    private long primitiveLongProperty = Long.MAX_VALUE;
    private Short shortProperty = Short.MAX_VALUE;
    private short primitiveShortProperty = Short.MAX_VALUE;
    private Byte byteProperty = Byte.MAX_VALUE;
    private byte primitiveByteProperty = Byte.MAX_VALUE;
    private Boolean booleanProperty = true;
    private boolean primitiveBooleanProperty = true;
    private BigDecimal bigDecimalProperty = sampleBigDecimalProperty;
    private BigInteger bigIntegerProperty = sampleBigIntegerProperty;

    public TestPojo() {}

    public TestPojo(String stringProperty, TestPojoInner pojoProperty) {
        this.stringProperty = stringProperty;
        this.pojoProperty = pojoProperty;
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
}
