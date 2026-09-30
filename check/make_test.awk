#!/usr/bin/gawk -f
BEGIN{
    print("#include \"unit_tests.h\"");
    print("int main(void) {");
    print("  int failed = 0;");
}
/START_TEST/{a = substr($1,12,length($1)-12); printf("  failed += %s();\n", a);}
END{
    print("  return failed != 0;");
    print("}");
}
