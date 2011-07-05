/*
<generator>
	<handlers doPostOut="doPostOut" doPostIn="doPostIn"/>
	<content-types>
		<content-type type="text/plain"/>
	</content-types>
</generator>
*/

/* file structure */
struct file_t {
	char filename[20];
	char data[100]
};

/* files structure */
struct files_t {
	struct file_t files[3];
	uint8_t index;
};

static char doPostIn(uint8_t content_type, uint8_t call_number, char *filename, void **post_data) {
	uint8_t i = 0;
	short value;

	/* no file */
	if(!filename)
		return 1;

	/* too many files (only 3 managed) */
	if(call_number > 2)
		return 1;

	/* allocating files structure if necessary */
	if(!*post_data){
		*post_data = mem_alloc(sizeof(struct files_t));
		if(!*post_data)
			return 1;
		((struct files_t *)*post_data)->index = 1;
	}
	else
		((struct files_t *)*post_data)->index++;

	/* copying filename */
	i = 0;
	while(filename[i] != '\0' && i < 19)
		((struct files_t *)*post_data)->files[call_number].filename[i] = filename[i++];
	((struct files_t *)*post_data)->files[call_number].filename[i] = '\0';

	/* copying data */
	uint8_t max = 99;
	if(call_number % 2 != 0)
		max = 10;
	i = 0;

	while((value = in()) != -1 && i < max)
		((struct files_t *)*post_data)->files[call_number].data[i++] = value;
	((struct files_t *)*post_data)->files[call_number].data[i] = '\0';
	
	/* test return -1 */
	in();	
	in();	
	in();	
	
	return 1;
}

static char doPostOut(uint8_t content_type, void *post_data) {
	out_str("Alternates between reading all characters or only 10");
	if(post_data){
		uint8_t j;
		for(j = 0 ; j < 3 ; j++){
			if(!((struct files_t *)post_data)->files[j].filename)
				continue;
			uint8_t i;
			/* printing data */
			out_str("\nThe file \"");
			out_str(((struct files_t *)post_data)->files[j].filename);
			out_str("\" contains :\n");
			out_str(((struct files_t *)post_data)->files[j].data);
		}
		/* cleaning memory */
		mem_free(post_data,sizeof(struct files_t));
	}
	else
		out_str("No data file");
	return 1;
}
