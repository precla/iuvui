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
#define BUFSZSMALL 80

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
	int index;
} hwmon_list_t;

void savePowercapNextAsDouble(powercap_list_t *, int);
void print_cpufreq(int, char *, bool *);
bool get_hwmon(const char *, char *);
powercap_list_t *get_powercap(int *);
hwmon_list_t *get_coretemp(int *);
