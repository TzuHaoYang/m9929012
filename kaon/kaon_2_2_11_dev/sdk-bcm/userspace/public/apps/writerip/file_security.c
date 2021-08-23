#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>

#define BLOCK_SIZE 16
#define FREAD_COUNT 4096
#define KEY_BIT 256
#define IV_SIZE 16
#define KEY_SIZE 64
#define RW_SIZE 1
#define KEY_DATA_BUF 1024

unsigned char iv[IV_SIZE];
AES_KEY aes_key;

// file encryption & decryption public key value
static unsigned char public_key[]="ZEt$j694JcptbfJgMwXajgtFMJeXwZw3";

int aes_encrypt_file_public_key(const char* input_file, const char* output_file)
{
	FILE *inFp, *outFp;
	int len = 0;
	int padding_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_encrypt_key(public_key, KEY_BIT, &aes_key) == -1)		
	{
		printf("file_encrypt] ERROR : failed to set encrypt key\n");
		return -1;
	}
	
	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp) ) > 0 )
	{
		if(FREAD_COUNT != len)
		{
			break;
		}
	
		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_ENCRYPT);
		fwrite(buf, RW_SIZE, len, outFp);
	}
	
	padding_len=BLOCK_SIZE - len % BLOCK_SIZE;
	printf("enc padding len:%d\n",padding_len);
	memset(buf+len, padding_len, padding_len);
	
	AES_cbc_encrypt(buf, buf, len+padding_len, &aes_key, iv, AES_ENCRYPT);
	fwrite(buf, RW_SIZE, len+padding_len, outFp);
	
	fclose(inFp);
	fclose(outFp);
	
	return 0;

}

int aes_decrypt_file_public_key(const char* input_file, const char* output_file)
{
	FILE *inFp, *outFp;
	int len = 0;
	int total_size = 0;
	int save_len = 0;
	int write_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
        	fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_decrypt_key(public_key, KEY_BIT, &aes_key) == -1)
	{
		printf("[file_decrypt] ERROR : failed to set decrypt key\n");
		return -1;
	}

	fseek(inFp, 0 , SEEK_END);
	total_size = ftell(inFp);
	fseek(inFp, 0, SEEK_SET);
	printf("total_size %d\n", total_size);

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) >0 )
	{
		if(FREAD_COUNT == 0)
		{
			break;
		}

		save_len += len;
		write_len = len;

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_DECRYPT);
		if(save_len == total_size)
		{
			write_len = len - buf[len-1];
			printf("decryption padding size %d\n", buf[len-1]);
		}

		fwrite(buf, RW_SIZE, write_len, outFp);
//		printf("dec write_len : %d\n",write_len);
	}

	fclose(inFp);
	fclose(outFp);
#if 1 // REMOVE WRONG DECRYPTION FILE
	if(write_len <= 0)
	{
		printf("Wrong Decryption file!!!\n");
		remove(output_file);
		return -1;
	}
#endif

	return 0;

}

int read_file(const char* input_file_name, unsigned char* buffer, int* buffer_length)
{
	int fd = 0;
	int file_size = 0;
	int readbytes = 0;
	struct stat file_state;

	fd = open(input_file_name, O_RDONLY, 0666);

	if (fd < 0)
	{
		fprintf(stderr, "[private_key_file_read] ERROR: Could not open file %s (%s)\n", input_file_name, strerror(errno));
		return -1;
	}

	stat(input_file_name, &file_state);
	file_size = file_state.st_size;

	do
	{
		readbytes += read(fd, buffer, file_size - readbytes);
	}while(readbytes < file_size);
	close(fd);

	printf("[read_file] file(%s), read_bytes(%d), file_size(%d)[%d:%s in %s]\n",
			input_file_name, readbytes, file_size, __LINE__, __FUNCTION__, __FILE__);
	*buffer_length = readbytes;

	return 0;
}

int aes_encrypt_file_key(const char* input_file, const char* output_file, unsigned char* input_key)
{
	FILE *inFp, *outFp;
	int len = 0;
	int padding_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_encrypt_key(input_key, KEY_BIT, &aes_key) == -1)
	{
		printf("file_encrypt] ERROR : failed to set encrypt key\n");
		return -1;
	}

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) > 0)
	{
		if(FREAD_COUNT != len)
		{
			break;
		}

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_ENCRYPT);
		fwrite(buf, RW_SIZE, len, outFp);
	}

    	padding_len=BLOCK_SIZE - len % BLOCK_SIZE;
    	printf("enc padding len:%d\n",padding_len);
    	memset(buf+len, padding_len, padding_len);

	AES_cbc_encrypt(buf, buf, len+padding_len, &aes_key, iv, AES_ENCRYPT);
	fwrite(buf, RW_SIZE, len+padding_len, outFp);

	fclose(inFp);
	fclose(outFp);

	return 0;
}

int aes_decrypt_file_key(const char* input_file, const char* output_file, unsigned char* input_key)
{
	FILE *inFp, *outFp;
	int len = 0;
	int total_size = 0;
	int save_len = 0;
	int write_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

        if((outFp = fopen(output_file, "wb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_decrypt_key(input_key, KEY_BIT, &aes_key) == -1)
	{
		printf("[file_decrypt] ERROR : failed to set decrypt key\n");
		return -1;
	}

	fseek(inFp, 0 , SEEK_END);
	total_size = ftell(inFp);
	fseek(inFp, 0, SEEK_SET);
	printf("total_size %d\n", total_size);

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) >0 )
	{
		if(FREAD_COUNT == 0)
		{
			break;
		}

		save_len += len;
		write_len = len;

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_DECRYPT);
		if(save_len == total_size)
		{
			write_len = len - buf[len-1];
			printf("decryption padding size %d\n", buf[len-1]);
		}

		fwrite(buf, RW_SIZE, write_len, outFp);
		printf("dec write_len : %d\n",write_len);
	}

	fclose(inFp);
	fclose(outFp);
#if 1 // REMOVE WRONG DECRYPTION FILE
	if(write_len <= 0)
	{
		printf("Wrong Decryption file!!!\n");
		remove(output_file);
		return -1;
	}
#endif
	return 0;
}

int aes_encrypt_file(const char* input_file, const char* output_file, const char* key_file)
{
	FILE *inFp, *outFp;
	int keylen = 0;
	int len = 0;
	int padding_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	unsigned char key[KEY_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	if (read_file(key_file, key, &keylen) < 0)
	{
		fprintf(stderr, "ERROR: Could not open file %s (%s)\n", key_file,strerror(errno));
		return -1;
	}

	memset(iv,0,sizeof(iv));

	if(AES_set_encrypt_key(key, KEY_BIT, &aes_key) == -1)
	{
		printf("file_encrypt] ERROR : failed to set encrypt key\n");
		return -1;
	}

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) > 0)
	{
		if(FREAD_COUNT != len)
		{
			break;
		}

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_ENCRYPT);
		fwrite(buf, RW_SIZE, len, outFp);
	}

    	padding_len=BLOCK_SIZE - len % BLOCK_SIZE;
    	printf("enc padding len:%d\n",padding_len);
    	memset(buf+len, padding_len, padding_len);

	AES_cbc_encrypt(buf, buf, len+padding_len, &aes_key, iv, AES_ENCRYPT);
	fwrite(buf, RW_SIZE, len+padding_len, outFp);

	fclose(inFp);
	fclose(outFp);

	return 0;
}

int aes_decrypt_file(const char* input_file, const char* output_file, const char* key_file)
{
	FILE *inFp, *outFp;
	int keylen = 0;
	int len = 0;
	int total_size = 0;
	int save_len = 0;
	int write_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	unsigned char key[KEY_SIZE];

	if((inFp = fopen(input_file, "rb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

        if((outFp = fopen(output_file, "wb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

        if (read_file(key_file, key, &keylen) < 0)
        {
                fprintf(stderr, "ERROR: Could not open file %s (%s)\n", key_file,strerror(errno));
                return -1;
        }
	memset(iv,0,sizeof(iv));

	if(AES_set_decrypt_key(key, KEY_BIT, &aes_key) == -1)
	{
		printf("[file_decrypt] ERROR : failed to set decrypt key\n");
		return -1;
	}

	fseek(inFp, 0 , SEEK_END);
	total_size = ftell(inFp);
	fseek(inFp, 0, SEEK_SET);
	printf("total_size %d\n", total_size);

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) >0 )
	{
		if(FREAD_COUNT == 0)
		{
			break;
		}

		save_len += len;
		write_len = len;

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_DECRYPT);
		if(save_len == total_size)
		{
			write_len = len - buf[len-1];
			printf("decryption padding size %d\n", buf[len-1]);
		}

		fwrite(buf, RW_SIZE, write_len, outFp);
		printf("dec write_len : %d\n",write_len);
	}

	fclose(inFp);
	fclose(outFp);
#if 1 // REMOVE WRONG DECRYPTION FILE
	if(write_len <= 0)
	{
		printf("Wrong Decryption file!!!\n");
		remove(output_file);
		return -1;
	}
#endif
	return 0;
}

char *key_generation(char *first_data, char *second_data, char *third_data)
{
	char key_data[KEY_DATA_BUF]; // first_data + second_data
	char final_key_data[KEY_DATA_BUF]; // SHA256(key_data) + third_data

	//SHA256
	unsigned char digest[SHA256_DIGEST_LENGTH];
	static char first_key[SHA256_DIGEST_LENGTH*2+1]; // SHA256(key_data)
	static char key_val[SHA256_DIGEST_LENGTH*2+1]; // SHA256(final_key)

	memset(key_data,0,sizeof(key_data));
	memset(final_key_data,0,sizeof(final_key_data));

	sprintf(key_data,"%s%s",first_data,second_data);

	// SHA256 : key_data
	SHA256_CTX first_ctx;
	SHA256_Init(&first_ctx);
	SHA256_Update(&first_ctx, key_data, strlen(key_data));
	SHA256_Final(digest, &first_ctx);

	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&first_key[i*2], "%02x", (unsigned int)digest[i]);
	}

	// SHA256 : final_key_data == generation key value
	sprintf(final_key_data,"%s%s",first_key,third_data);
	printf("final_key_data : %s\n",final_key_data);

	SHA256_CTX key_ctx;
	SHA256_Init(&key_ctx);
	SHA256_Update(&key_ctx,final_key_data,strlen(final_key_data));
	SHA256_Final(digest, &key_ctx);

	for(int j = 0; j < SHA256_DIGEST_LENGTH; j++)
	{
		sprintf(&key_val[j*2], "%02x", (unsigned int)digest[j]); 
	}

	return key_val;	
}

char *pw_generation(char *first_data, char *second_data, char *third_data, char *time_data)
{
	char key_data[KEY_DATA_BUF]; // first_data + second_data
	char final_key_data[KEY_DATA_BUF]; // SHA256(key_data) + third_data
	char pw_key_data[KEY_DATA_BUF]; // SHA256(key_data) + third_data

	//SHA256
	unsigned char digest[SHA256_DIGEST_LENGTH];
	static char first_key[SHA256_DIGEST_LENGTH*2+1]; // SHA256(key_data)
	static char key_val[SHA256_DIGEST_LENGTH*2+1]; // SHA256(final_key)
	static char pw_val[SHA256_DIGEST_LENGTH*2+1]; // SHA256(final_key)

	memset(key_data,0,sizeof(key_data));
	memset(final_key_data,0,sizeof(final_key_data));

	sprintf(key_data,"%s%s",first_data,second_data);

	// SHA256 : key_data
	SHA256_CTX first_ctx;
	SHA256_Init(&first_ctx);
	SHA256_Update(&first_ctx, key_data, strlen(key_data));
	SHA256_Final(digest, &first_ctx);

	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&first_key[i*2], "%02x", (unsigned int)digest[i]);
	}

	// SHA256 : final_key_data == generation key value
	sprintf(final_key_data,"%s%s",first_key,third_data);
	printf("final_key_data : %s\n",final_key_data);

	SHA256_CTX key_ctx;
	SHA256_Init(&key_ctx);
	SHA256_Update(&key_ctx,final_key_data,strlen(final_key_data));
	SHA256_Final(digest, &key_ctx);

	for(int j = 0; j < SHA256_DIGEST_LENGTH; j++)
	{
		sprintf(&key_val[j*2], "%02x", (unsigned int)digest[j]);
	}

	// SHA256 : final_key_data == generation key value
	sprintf(pw_key_data,"%s%s",key_val,time_data);
	printf("pw_key_data : %s\n",pw_key_data);

	SHA256_CTX pw_ctx;
	SHA256_Init(&pw_ctx);
	SHA256_Update(&pw_ctx,pw_key_data,strlen(pw_key_data));
	SHA256_Final(digest, &pw_ctx);

	for(int k = 0; k < SHA256_DIGEST_LENGTH; k++)
	{
		sprintf(&pw_val[k*2], "%02x", (unsigned int)digest[k]);
	}

	return pw_val;
}

char *pw_generation2(char *first_data, char *time_data)
{
	char key_data[KEY_DATA_BUF]; // first_data + time_data

	//SHA256
	unsigned char digest[SHA256_DIGEST_LENGTH];
	static char first_key[SHA256_DIGEST_LENGTH*2+1]; // SHA256(key_data)

	memset(key_data,0,sizeof(key_data));

	sprintf(key_data,"%s%s",first_data,time_data);

	printf("key_data:%s\n",key_data);

	// SHA256 : key_data
	SHA256_CTX first_ctx;
	SHA256_Init(&first_ctx);
	SHA256_Update(&first_ctx, key_data, strlen(key_data));
	SHA256_Final(digest, &first_ctx);

	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(&first_key[i*2], "%02x", (unsigned int)digest[i]);
	}

	return first_key;
}

char *file_to_data(const char* input_file)
{
	// -------- convert ------------
	int readfile, writefile;
	unsigned char readbuf;
	unsigned char writebuf[BUFSIZ];
	ssize_t readb;
	int i = 0;

	// ------- file to data --------
	FILE *returnFp;
	int return_len =0;
	unsigned char databuf[15360];
	static char returnbuf[15360];

	memset(databuf,0,sizeof(databuf));
	memset(returnbuf, 0, sizeof(returnbuf));

	// input file read date write
	readfile = open(input_file, O_RDONLY);

	if(readfile == -1)
	{
		printf("data input file open error\n");
	}

	writefile = open("./data.txt", O_WRONLY|O_TRUNC|O_CREAT, 0666);
	if(writefile == -1)
	{
		perror("error : ");
		printf("data write file open error\n");
	}

	while((readb = read(readfile, &readbuf, 1)) > 0)
	{
		if(i != 0 && (i%50) == 0)
		{
			write(writefile, "\n", 1);
		}
		sprintf(writebuf, "%02x", readbuf);
		write(writefile, writebuf, 2);
		i++;
	}

	close(readfile);
	close(writefile);

	// write file data return
	returnFp = fopen("./data.txt","rb");
	if(returnFp == NULL)
	{
		printf("data file open error\n");
	}

	while((return_len = fread(databuf, 1, sizeof(databuf), returnFp)) > 0)
	{
		if(return_len == 0)
		{
			printf("return len error\n");
			break;
		}
	}
	printf("databuf : %s\n",databuf);
	sprintf(returnbuf, "%s", databuf);
	printf("return val : %s\n",returnbuf);
	
	fclose(returnFp);	

	return returnbuf;
	
}

char *aes_encrypt_file_to_data_public_key(const char* input_file, const char* output_file)
{
	FILE *inFp, *outFp;
	int len = 0;
	int padding_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	char *data_val;

	if((inFp = fopen(input_file, "rb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_encrypt_key(public_key, KEY_BIT, &aes_key) == -1) // public key test
	{
		printf("file_encrypt] ERROR : failed to set encrypt key\n");
	}

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) > 0)
	{
		if(FREAD_COUNT != len)
		{
			break;
		}

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_ENCRYPT);
		fwrite(buf, RW_SIZE, len, outFp);
	}

    	padding_len=BLOCK_SIZE - len % BLOCK_SIZE;
    	printf("enc padding len:%d\n",padding_len);
    	memset(buf+len, padding_len, padding_len);

	AES_cbc_encrypt(buf, buf, len+padding_len, &aes_key, iv, AES_ENCRYPT);
	fwrite(buf, RW_SIZE, len+padding_len, outFp);

	fclose(inFp);
	fclose(outFp);

	data_val = file_to_data(output_file);
	printf("data_val : %s\n",data_val);
	return data_val;
}

char *aes_decrypt_file_to_data_public_key(const char* input_file, const char* output_file)
{
	FILE *inFp, *outFp;
	int len = 0;
	int total_size = 0;
	int save_len = 0;
	int write_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	char *dec_data_val;


	if((inFp = fopen(input_file, "rb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

        if((outFp = fopen(output_file, "wb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	memset(iv,0,sizeof(iv));

	if(AES_set_decrypt_key(public_key, KEY_BIT, &aes_key) == -1)		
	{
		printf("[file_decrypt] ERROR : failed to set decrypt key\n");
	}

	fseek(inFp, 0 , SEEK_END);
	total_size = ftell(inFp);
	fseek(inFp, 0, SEEK_SET);
	printf("total_size %d\n", total_size);

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) >0 )
	{
		if(FREAD_COUNT == 0)
		{
			break;
		}

		save_len += len;
		write_len = len;

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_DECRYPT);
		if(save_len == total_size)
		{
			write_len = len - buf[len-1];
			printf("decryption padding size %d\n", buf[len-1]);
		}

		fwrite(buf, RW_SIZE, write_len, outFp);
		printf("dec write_len : %d\n",write_len);
	}

	fclose(inFp);
	fclose(outFp);

	dec_data_val = file_to_data(output_file);
	printf("dec_data_val : %s\n",dec_data_val);
	return dec_data_val;

}

char *aes_encrypt_file_to_data(const char* input_file, const char* output_file, const char* key_file)
{
	FILE *inFp, *outFp;
	int keylen = 0;
	int len = 0;
	int padding_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	unsigned char key[KEY_SIZE];
	char *data_val;

	if((inFp = fopen(input_file, "rb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

	if((outFp = fopen(output_file, "wb")) == NULL)
		fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

	if (read_file(key_file, key, &keylen) < 0)
	{
		fprintf(stderr, "ERROR: Could not open file %s (%s)\n", key_file,strerror(errno));
		//return -1;
	}

	memset(iv,0,sizeof(iv));

	if(AES_set_encrypt_key(key, KEY_BIT, &aes_key) == -1)
	{
		printf("file_encrypt] ERROR : failed to set encrypt key\n");
		//return -1;
	}

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) > 0)
	{
		if(FREAD_COUNT != len)
		{
			break;
		}

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_ENCRYPT);
		fwrite(buf, RW_SIZE, len, outFp);
	}

    	padding_len=BLOCK_SIZE - len % BLOCK_SIZE;
    	printf("enc padding len:%d\n",padding_len);
    	memset(buf+len, padding_len, padding_len);

	AES_cbc_encrypt(buf, buf, len+padding_len, &aes_key, iv, AES_ENCRYPT);
	fwrite(buf, RW_SIZE, len+padding_len, outFp);

	fclose(inFp);
	fclose(outFp);

	data_val = file_to_data(output_file);
	printf("data_val : %s\n",data_val);
	return data_val;
}

char *aes_decrypt_file_to_data(const char* input_file, const char* output_file, const char* key_file)
{
	FILE *inFp, *outFp;
	int keylen = 0;
	int len = 0;
	int total_size = 0;
	int save_len = 0;
	int write_len = 0;
	unsigned char buf[FREAD_COUNT+BLOCK_SIZE];
	unsigned char key[KEY_SIZE]; 
	char *dec_data_val;


	if((inFp = fopen(input_file, "rb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",input_file,strerror(errno));

        if((outFp = fopen(output_file, "wb")) == NULL)
                fprintf(stderr, "ERRPR: Could not open file %s (%s)\n",output_file,strerror(errno));

        if (read_file(key_file, key, &keylen) < 0)
        {
                fprintf(stderr, "ERROR: Could not open file %s (%s)\n", key_file,strerror(errno));
                //return -1;
        }

	memset(iv,0,sizeof(iv));

	if(AES_set_decrypt_key(key, KEY_BIT, &aes_key) == -1)
	{
		printf("[file_decrypt] ERROR : failed to set decrypt key\n");
		//return -1;
	}

	fseek(inFp, 0 , SEEK_END);
	total_size = ftell(inFp);
	fseek(inFp, 0, SEEK_SET);
	printf("total_size %d\n", total_size);

	while((len = fread(buf, RW_SIZE, FREAD_COUNT, inFp)) >0 )
	{
		if(FREAD_COUNT == 0)
		{
			break;
		}

		save_len += len;
		write_len = len;

		AES_cbc_encrypt(buf, buf, len, &aes_key, iv, AES_DECRYPT);
		if(save_len == total_size)
		{
			write_len = len - buf[len-1];
			printf("decryption padding size %d\n", buf[len-1]);
		}

		fwrite(buf, RW_SIZE, write_len, outFp);
		printf("dec write_len : %d\n",write_len);
	}

	fclose(inFp);
	fclose(outFp);

	dec_data_val = file_to_data(output_file);
	printf("dec_data_val : %s\n",dec_data_val);
	return dec_data_val;

}
