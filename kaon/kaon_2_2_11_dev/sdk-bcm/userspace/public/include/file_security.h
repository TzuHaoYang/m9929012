#ifndef __FILE_SECURITY_H__
#define __FILE_SECURITY_H__

int aes_encrypt_file_public_key(const char* input_file, const char* output_file);
int aes_decrypt_file_public_key(const char* input_file, const char* output_file);
int aes_encrypt_file_key(const char* input_file, const char* output_file, unsigned char* input_key);
int aes_decrypt_file_key(const char* input_file, const char* output_file, unsigned char* input_key);
int read_file(const char* input_file_name, unsigned char* buffer, int* buffer_length);
int aes_encrypt_file(const char* input_file, const char* output_file, const char* key_file);
int aes_decrypt_file(const char* input_file, const char* output_file, const char* key_file);
char *key_generation(char *first_data, char *second_data, char *third_data);
char *pw_generation(char *first_data, char *second_data, char *third_data, char *time_data);
char *pw_generation2(char *first_data, char *time_data);
char *file_to_data(const char* input_file);
char *aes_encrypt_file_to_data_public_key(const char* input_file, const char* output_file);
char *aes_decrypt_file_to_data_public_key(const char* input_file, const char* output_file);
char *aes_encrypt_file_to_data(const char* input_file, const char* output_file, const char* key_file);
char *aes_decrypt_file_to_data(const char* input_file, const char* output_file, const char* key_file);

#endif /* __FILE_SECURITY_H__ */
