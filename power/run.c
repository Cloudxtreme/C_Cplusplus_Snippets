#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int COUNT = 288;

void itoa (int num, char * ch) {
  int i=0, j;
  char tmp;
  if (num == 0) {
    ch[0] = '0';
    ch[1] = '\0';
    return;
  }
  while (num != 0) {
    ch[i] = '0' + num%10;
    i++;
    num=num/10;
  }
  ch[i]='\0';
  for (j=0; j<i/2; j++) {
    tmp = ch[j];
    ch[j]=ch[i-1-j];
    ch[i-1-j]=tmp;
  }
}

int isExisted (char *FILENAME) {
  if (!access(FILENAME, F_OK)) {
    return 1;
  }
  
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s source.f\n", argv[0]);
    exit(0);
  }

  char *infilename = NULL;
  //  char *pid = NULL;
  infilename = strdup(argv[1]);
  //  pid = strdup(argv[2]);
  char comma[4] = ".";

  int i;
  FILE *fp;
  char isLocked;
  fp = fopen("./lock","w");
  fputc('C', fp); // closed
  fclose(fp);

  for (i=0; i<COUNT; i++) {
    char pathname[64] = "./uhf90/";
    strcat(pathname, infilename);
    strcat(pathname, comma);
    char tmp_num[8];
    itoa(i, tmp_num);
    strcat(pathname, tmp_num);
   
    if (isExisted(pathname)) {
      // after executing, wait_unlock
      while (1) {
	fp = fopen("./lock","r");
	isLocked = fgetc(fp);
	fclose(fp);

	if (isLocked == 'O') { // unlocked
	  break;
	} else { // still closed
	  sleep(1);
	}
      }

      // before executing, lock
      fp = fopen("./lock","w");
      fputc('C', fp); // closed
      fclose(fp);

      system(pathname);

      fp = fopen("./lock","w");
      fputc('W', fp); // tell sample to wait
      fclose(fp);
    }/* else { // else compile failed
	printf("%s not exists!\n", pathname);
	}*/
    sleep(1);
  }
  
  //  char kill[32] = "/bin/kill ";
  //  strcat(kill, pid);
  //  system(kill);
  return 0;
}
