#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "string_manip.h"
#include "utils.h"

int convert_string_to_double(const char* str, double *out) {

	char *endptr;
	*out = strtod(str, &endptr);		

	if (endptr == str) { 
		SGEN_WARNING("strtod: no conversion performed! (input string: \"%s\")\n", str);
		*out = 0;
		return -1; 	
	}

	if (endptr && *endptr != '\0') {
		SGEN_WARNING("strtod: full double conversion of string \"%s\" couldn't be performed\n", str);
		return 0;
	}
	
	return 1;
}

static void find_stuff_between_errmsg(char beg, char end, int error) {
	error *= -1;
	if (error & 0x1) {
		SGEN_ERROR("beginning delimiter char (\'%c\') not found in input string.\n", beg);
	}
	if (error & 0x2) {
		if (error == 0x2) {
			SGEN_ERROR("unmatched delimiter \'%c\'! (expected a \'%c\'\n", beg, end);
		}
		else {
			SGEN_ERROR("ending delimiter char (\'%c\') not found in input string.\n", end);
		}
	}
	if (error & 0x4) {
		SGEN_ERROR("ending token (\'%c\') encountered before beginning token (\'%c\')!\n", end, beg);
	}

}

int find_stuff_between(char beg, char end, char* input, char** output) {

	char *block_beg = strchr(input, beg); 
	char *block_end = strrchr(input, end); 

	int error = 0;

	if (!block_beg && !block_end) {
		return 0;
	}

	if (!block_beg) {
		error |= 0x1;
	}
	if (!block_end) { 
		error |= 0x2;
	}

	if (block_beg >= block_end) {
		error |= 0x4;
	}

	if (error) {
		SGEN_ERROR("erroneous input:\"\n%s\n\", delims = %c, %c. error code %x\n", input, beg, end, error);
		error *= -1;
		find_stuff_between_errmsg(beg, end, error);
		return error;
	}

	long b = block_beg - input;
	long nc = block_end - block_beg - 1;

	*output = substring(input, b+1, nc); // input[(block_beg+1)..block_end];
	return 1;

}


	
