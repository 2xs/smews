#ifndef __EXPORTS_H__
#define __EXPORTS_H__

// Global variable
extern volatile int32_t global_timer;

#ifdef __cplusplus
	extern "C" {
#endif

	int16_t dev_get();
	char dev_data_to_read(void);

	void dev_prepare_output(void);
	void dev_put(unsigned char c);
	void dev_output_done(void);

#ifdef __cplusplus
	}
#endif

#endif


