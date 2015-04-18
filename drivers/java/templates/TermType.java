public enum TermType {

    public final int value;

    private TermType(int value){
        this.value = value;
    }
%for term, value in term_types.items():
    ${term}(${value}),
%endfor
}
