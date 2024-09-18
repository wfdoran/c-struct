#!/usr/bin/gawk -f
BEGIN{
    print("#include \"unit_tests.h\"");
    print("int main(void) {");
}
/START_TEST/{a = substr($1,12,length($1)-12); printf("  %s();\n", a);}
END{
    print("  return 0;");
    print("}");
}
