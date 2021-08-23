#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <mtd/mtd-user.h>
#include <writerip_priv.h>
#include <file_security.h>

static void usage(char* name)
{
    fprintf(stdout,
            "Usage: %s [-iwsRrh] [args]\n"
            "\t-i: Initialized the RIP with default values\n"
            "\t-b: Backup RIP content in files (%s -b /mnt/usb/backup)\n"
            "\t-w: Write RIP using data from files (%s -w /mnt/usb/)\n"
            "\t    Backup files of the previous settings (if any) will\n"
            "\t    be stored in a backup directory under the directory\n"
            "\t    in argument\n"
            "\t    Filenames:\n"
            "\t\tSerial number: serial_number.txt\n"
            "\t\tWAN address: mac_address.txt\n"
            "\t\tManufacturer: manufacturer.txt\n"
            "\t\tManufacturer OUI: manufacturer_oui.bin\n"
            "\t\tManufacturer URL: manufacturer_url.txt\n"
            "\t\tModel name: model_name.txt\n"
            "\t\tProduct Class: product_class.txt\n"
            "\t\tHW version: hardware_version.txt\n"
            "\t\tBootloader version: bootloader_version.txt\n"
            "\t\tPublic RSA key (norm): public_dsa_norm\n"
            "\t\tPublic RSA key (resc): public_dsa_resc\n"
            "\t\tROOT certificate: root_certificate\n"
            "\t\tClient certificate: client_certificate\n"
            "\t\tPrivate key: private_key\n"
            "\t\tWLAN key: wlan_key.txt\n"
            "\t\tWLAN SROM map 1: wlan_srom_1.bin\n"
            "\t\tWLAN SROM map 2: wlan_srom_2.bin\n"
            "\t-s: Update a specific field in the RIP (not yet supported)\n"
            "\t-R: Read all data from the RIP\n"
            "\t-r: Read a specific field from the RIP\n"
            "\t-l: List all valid names\n"
            "\t-v: Version\n"
            "\t-h: Display this help and exit\n"
            "\n%s: version %u.%u\n",
            name, name, name, name, VERSION_MAJOR, VERSION_MINOR);
}

int main(int argc, char *argv[])
{
	int opt;
	char dir[1024];
	RIP_ACTIONS_E action=RIP_NOACTION;

	if(argc < 2)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt(argc, argv, "ib:w:s:d:e:Rr:pfclvh")) != -1)
	{
		switch (opt)
		{
			case 'i':
				action = RIP_SET_DEFAULT;
				break;
			case 'b':
				action = RIP_DUMP_TOFILE;
			strncpy(dir, optarg, sizeof(dir));
				break;
			case 'w':
				action = RIP_SET_FILES;
				strncpy(dir, optarg, sizeof(dir));
				break;
			case 's':
				action = RIP_SET_FIELD;
				strncpy(dir, optarg, sizeof(dir));
				break;
				//fprintf(stdout,"This is option is not yet implemented\n");
				//usage(argv[0]);
				//exit(EXIT_SUCCESS);
			case 'd':
				action = RIP_DEC_FW;
				strncpy(dir, optarg, sizeof(dir));
				break;
			case 'e':
				action = ENC_CERTI;
				strncpy(dir, optarg, sizeof(dir));
				break;
			case 'r':
				action = RIP_GET_FIELD;
				break;
			case 'R':
				action = RIP_GET_ALL;
				break;
			case 'f':
				action = RIP_TO_NVRAM;
				break;
			case 'c':
				action = DECRYPT_CERTI;
				break;
			case 'p':
				action = CHANGE_PW;
				break;
			case 'l':
				action=RIP_LIST_FIELDS;
				break;
			case 'v':
				fprintf(stdout, "%s: version %u.%u\n", argv[0],
				VERSION_MAJOR, VERSION_MINOR);
			break;
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	switch(action)
	{
		case RIP_SET_DEFAULT:
			if (-1 == set_default_rip()) {
				fprintf(stderr, "Setting default values into the RIP failed\n");
				exit(EXIT_FAILURE);
			}
			fprintf(stdout, "Default values have been written successfully into the RIP\n");
			break;

		case RIP_DUMP_TOFILE:
		{
			int ret = dump_rip_to_files(dir);
			if (-1 == ret) {
				fprintf(stderr, "Failed dumping values from RIP\n");
				exit(EXIT_FAILURE);
			}
			else if (0 == ret) /* RIP is empty */
				fprintf(stdout, "RIP is erased => nothing to dump\n");
			else
				fprintf(stdout, "RIP values were dump to %s\n", dir);
			break;
		}
		case RIP_SET_FILES:
			if (-1 == write_rip_use_files(dir)) {
				fprintf(stderr, "Setting values using files into the RIP failed\n");
				exit(EXIT_FAILURE);
			}
			fprintf(stdout, "RIP values have been written successfully into the RIP\n");
			break;
		case RIP_DEC_FW:
			if(-1 == aes_decrypt_file_public_key(dir,"/tmp/dec_fw")){
				fprintf(stderr, "FW file decrypt failed\n");
				exit(EXIT_FAILURE);
			}
			fprintf(stderr, "FW file decrypt success\n");
			break;
		case ENC_CERTI:
		#if defined(CONFIG_CUSTOM_PLUME)
			fprintf(stderr, "CONFIG_CUSTOM_PLUME success\n");
			encrypt_certi_file(dir);
		#else
			fprintf(stderr, "CONFIG_CUSTOM_PLUME fail\n");
		#endif
			break;
		case RIP_LIST_FIELDS:
			rip_list_fields();
			break;

		case RIP_GET_ALL:
			if(!print_mtd_rip(NULL))
				fprintf(stdout, "RIP partition is empty\n");
			break;

		case RIP_GET_FIELD:
			if(!print_mtd_rip(argv[2]))
				fprintf(stdout, "RIP partition is empty\n");
			break;
		case RIP_SET_FIELD:
			if(-1 == write_rip_field(argv[2], argv[3])){
				fprintf(stderr, "Setting value into the RIP failed\n");
				exit(EXIT_FAILURE);
			}
			fprintf(stdout, "RIP value have been written successfully into the RIP\n");
			break;
		case RIP_TO_NVRAM:
			rip_to_nvram();
			fprintf(stdout, "Factory default,RIP data into the NVRAM\n");
			break;
		case DECRYPT_CERTI:
			if(decrypt_certi_file() == 0)
				fprintf(stdout, "Decrypt Certificate file\n");
			else
				fprintf(stdout, "Fail Decrypt Certificate file\n");
			break;
		case CHANGE_PW:
			change_ssh_pw2();
			break;
		default:
			fprintf(stderr, "Unknown action %d", action);
			exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}



