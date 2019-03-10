/* taken from https://github.com/kitsunyan/intel-undervolt
 * and made a few modifications
 */

#include <dirent.h>
#include <fcntl.h>
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DIR_POWERCAP "/sys/class/powercap"
#define DIR_HWMON "/sys/class/hwmon"
#define BUFSZSMALL 512

typedef struct {
	void * next;
	char * name;
	char * dir;
	int64_t last;
    double val;
	struct timespec time;
} powercap_list_t;

typedef struct {
	void * next;
	char * name;
	char * dir;
	char val[BUFSZSMALL];
	int index;
} hwmon_list_t;

void savePowercapNextAsDouble(powercap_list_t *, int);
void printCpufreq(int, char *, bool *);
void getHwmonNextValues(hwmon_list_t *, int, char *);
bool getHwmon(const char *, char *);
powercap_list_t *getPowercap(int *);
hwmon_list_t *getCoretemp(int *);
