#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CONFFILE        "/etc/intel-undervolt.conf"

/* count elements below */
#define COUNTELEMENTS   6

#define CPUOFFSET       0
#define GPUOFFSET       1
#define CPUCACHEOFFSET  2
#define SYSAOFFSET      3
#define ANALOGIOOFFSET  4
#define DAEMONINTERVAL  5

/* max number of digits */
#define MAXDIGIT        6
/* Length for undervolt option */
#define UV_OPTION       64

#define MAXLENSENTENCE  128

/* max and minimal voltage offset */
#define MAXVOLTAGEOFF   100
#define MINVOLTAGEOFF   -250

#define BUFFERSIZE      4096

#define VOLTAGESTEP     1

/*
 * Set the value into the config file /etc/intel-undervolt.conf
 * and apply it. Returns 0 on success
 */
int setValIntoConfFile(int, short);

/*
 * Returns 0 on success, -1 on error
 * TODO: add support for the following settings:
 *    power package ..
 *    tjoffset
 *    interval
 */
int readVal(int *);

/* 
 * applyValues - set values into conf file and apply.
 * Also compares with current values in the conf file
 * /etc/intel-undervolt.conf , if the value is the same,
 * no change will be made to that one.
 * Returns how many values have been changed.
 */
short applyValues(int *, int *);

void measurePowerConsumption();
int daemonMode();

/*
 * Set Power Limits Alteration
 * see config:
 *     # Usage: power ${domain} ${short_power_value} ${long_power_value}
 *     # Power value: ${power}[/${time_window}][:enabled][:disabled]
 *     # Supported domains: package
 *        power package 45 35
 *        power package 45/0.002 35/28
 *        power package 45/0.002:disabled 35/28:enabled
 * Returns 0 on success, -1 on error
 */
int powerLimitAlt(float, float);

/*
 * Set Critical Temperature Offset Alteration
 * see config:
 *    # Usage: tjoffset ${temperature_offset}
 *        tjoffset -20
 * Returns 0 on success, -1 on error
 */
int tempOffsetAlt(int);

/*
 * Set Daemon Update Interval
 * see config:
 *    # Usage: interval ${interval_in_milliseconds}
 *        interval 5000
 * Returns 0 on success, -1 on error
 */
int daemonUpdateInterval(unsigned int);

/*
 * Enable/disable daemon.
 * int set = 0 -> disable,
 * int set = 1 -> enable,
 * int set = 2 -> check if enabled,
 * returns 0 on success, -1 on error
 */
int systemdService(int);

/*
 * Resets all Values to 0
 * Returns 0 on success, -1 on error
 */
int uvResetAll();

/*
 * convert int to string
 */
void intToString(int , char *, char *);
