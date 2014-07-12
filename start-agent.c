#include <stdio.h>
#include <string.h>
#define CMD_SIZE 128
#define BUF_SIZE 1024


/*
* wait for given output for given os command
* return 0: if not found, or no output from command
* return +n : number of occurences found
* return -1 : a NULL is expected but command has some output.
*/
int expect(char *cmd, char *out_str)
{
	char buff[BUF_SIZE];
	int found = 0;
	//printf("%s\n", cmd);
	FILE *fp = popen( cmd, "r" );
	if (out_str != NULL)
	{
		while ( fgets( buff, sizeof buff, fp ) != NULL ) {
  			printf("::%s\n", buff);
			if (strstr(buff, out_str) != NULL)
			found++;
			continue;
		}
	} else {
		while ( fgets( buff, sizeof buff, fp ) != NULL ) {
 			printf("::%s\n", buff);
			return -1;
		}
	}
	pclose(fp);
	return found;
}


int main(int argc, char argv[])
{
	char cmd[CMD_SIZE], str[CMD_SIZE];
    int res= 0;

    sprintf(cmd, "hciconfig hci0 encrypt auth piscan");
	
	res = expect(cmd, NULL);
	printf("%s\n", res);
    printf("res = %d\n", res);

    sprintf(cmd, "rfcomm bind hci0 %s %s\n", argv[1], "1");
    printf("%s\n",cmd);
	res = expect(cmd, NULL);
    printf("res = %d\n", res);

    sprintf(cmd, "rfcomm show");
    printf("%s\n",cmd);
	res = expect(cmd, NULL);
    printf("res = %d\n", res);



	return 0;

}
