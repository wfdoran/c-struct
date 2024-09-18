#!/usr/bin/gawk -f
BEGIN{
}
/START_TEST/{a = substr($1,12,length($1)-12); printf("int %s();\n", a);}
END{
}
