public enum ${classname} {

    public final int value;

    private ${classname}(int value){
        this.value = value;
    }
%for key, value in items:
    ${key}(${value}),
%endfor
}
