/*
<generator>
	<handlers doGet="doGetApplis"/>
	<properties persistence="idempotent"/>
</generator>
*/

#include "../../core/elf_application.h"

static CONST_VAR(unsigned char, fields_names[5][16]) = {
	{"name"},
	{"size"},
};

static CONST_VAR(unsigned char, str0[]) = "[";
static CONST_VAR(unsigned char, str1[]) = "{";
static CONST_VAR(unsigned char, str2[]) = ":\"";
static CONST_VAR(unsigned char, str3[]) = "\",";
static CONST_VAR(unsigned char, str4[]) = "},";
static CONST_VAR(unsigned char, str5[]) = "]";

static void out_const_str(const unsigned char /*CONST_VAR*/ *str) {
	const unsigned char *c = str;
	unsigned char tmp;
	while((tmp = CONST_READ_UI8(c++))!='\0'){
		out_c(tmp);
	}
}

static char doGetApplis(struct args_t *args) {
	out_const_str(str0);
	
	FOR_EACH_APPLICATION(appli, 

		out_const_str(str1);
		
		out_const_str(fields_names[0]);
		out_const_str(str2);
		out_const_str(appli->filename);
		out_const_str(str3);

		out_const_str(fields_names[1]);
		out_const_str(str2);
		out_uint(appli->size);
		out_const_str(str3);		

		out_const_str(str4);
	)

	out_const_str(str5);
	return 1;
}
