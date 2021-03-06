
#include<sys/time.h>
struct five_tuple_tmp
{
	/* data */
	char val[1600];
    int data_len;
	time_t tv_sec;
	suseconds_t tv_usec;
};

#ifdef __cplusplus

extern "C" {

#endif

int hashmap_size(unsigned int key);

void hashmap_insert(unsigned int key,struct five_tuple_tmp val);
void hashmap_clear(unsigned int key);
struct five_tuple_tmp hashmap_val(unsigned int key,int i);
void hashmap_clear(unsigned int key);
void fore_clear(char* file_path);
#ifdef __cplusplus

}

#endif