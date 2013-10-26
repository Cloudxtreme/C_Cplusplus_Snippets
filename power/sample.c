#include <stdio.h>

void get_power();

int main(int argc, char *argv[]) {
  char isLocked;
  FILE *fp;
  int i;

  while (1) { // closed
    fp = fopen("./lock", "w");
    fputc('O', fp); // tell sample to start
    fclose(fp);

    sleep(2); // in case just start

    while (1) {
      fp = fopen("./lock","r");
      isLocked = fgetc(fp);
      fclose(fp);

      if (isLocked == 'C') { // started
	for (i=0; i<8; i++) {
	  get_power();
	  //printf("get_power()\n");
	  sleep(1);
	}
	printf("\n");

	while (1) {
	  fp = fopen("./lock","r");
	  isLocked = fgetc(fp);
	  if (isLocked == 'W') { // start another round
	    fclose(fp);
	    break;
	  } else {
	    sleep(1);
	  }
	}
	break;
      } else {
	printf("isLocked = %c\n", isLocked);
	sleep(1);
      }
    }

  }

  return 0;
}
