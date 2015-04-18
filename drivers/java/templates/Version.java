public enum Version {

    public final int value;

    private Version(int value){
        this.value = value;
    }
%for term, value in versions.items():
    ${term}(${value}),
%endfor
}
