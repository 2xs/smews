/*
<generator>
	<handlers doPostOut="doPostOut" doPostIn="doPostIn"/>
	<content-types>
	</content-types>
</generator>
*/

static char doPostIn(uint8_t content_type, uint8_t part_number, char *filename, void **post_data) {
	uint8_t i = 0;
	short value;

	/* too many files (only 1 managed) */
	if(part_number > 1)
		return 1;

	/* allocating files structure if necessary */
	if(!*post_data){
		*post_data = mem_alloc(100 * sizeof(char));
		if(!*post_data)
			return 1;
	}

	i = 0;
	while((value = in()) != -1 && i < 99)
		((char *)*post_data)[i++] = value;
	((char *)*post_data)[i++] = '\0';

	/* test return -1 */
	in();	
	in();	
	in();	

	out_str("To test wrong usage\n"); // never called

	return 1;
}

static char doPostOut(uint8_t content_type, void *post_data) {
 	if(post_data){
		out_str((char *)post_data);
		mem_free(post_data,100 * sizeof(char));
	}
	else
		out_str("No data file");

	in(); // never called
	return 1;
}
