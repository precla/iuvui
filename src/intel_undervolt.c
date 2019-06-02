#include "intel_undervolt.h"

int setValIntoConfFile(int offset, short option) {
        char newConf[MAXLENSENTENCE];
        char tmp[MAXLENSENTENCE];
        char currConf[MAXLENSENTENCE];
        char *fileSize;
        struct stat buf; 

        fpos_t fpos;

        FILE *f = fopen(CONFFILE, "r+");

        if (!f) {
                return 1;
        }

        switch (option)
        {
                case CPUOFFSET:
                        strncpy(newConf, "undervolt 0 'CPU' ", 19);
                        intToString(offset, tmp, newConf);
                        break;
                case GPUOFFSET:
                        strncpy(newConf, "undervolt 1 'GPU' ", 19);
                        intToString(offset, tmp, newConf);
                        break;
                case CPUCACHEOFFSET:
                        strncpy(newConf, "undervolt 2 'CPU Cache' ", 25);
                        intToString(offset, tmp, newConf);
                        break;
                case SYSAOFFSET:
                        strncpy(newConf, "undervolt 3 'System Agent' ", 28);
                        intToString(offset, tmp, newConf);
                        break;
                case ANALOGIOOFFSET:
                        strncpy(newConf, "undervolt 4 'Analog I/O' ", 26);
                        intToString(offset, tmp, newConf);
                        break;
                case DAEMONINTERVAL:
                        strncpy(newConf, "interval ", 10);
                        intToString(offset, tmp, newConf);
                        break;
                default:
                        return 1;
        }
        strncat(newConf, "\n", 2);

        while (!feof(f)) {
                fgetpos(f, &fpos);
                fgets(currConf, MAXLENSENTENCE, f);

                if (strchr(currConf, '#') || strlen(currConf) < 8) {
                        continue;
                } else if (option == CPUOFFSET && strstr(currConf, "undervolt 0 \'CPU")) {
                        break;
                } else if (option == GPUOFFSET && strstr(currConf, "undervolt 1 \'GPU")) {
                        break;
                } else if (option == CPUCACHEOFFSET && strstr(currConf, "undervolt 2 \'CPU Cache")) {
                        break;
                } else if (option == SYSAOFFSET && strstr(currConf, "undervolt 3 \'System Agent")) {
                        break;
                } else if (option == ANALOGIOOFFSET && strstr(currConf, "undervolt 4 \'Analog I/O")) {
                        break;
                } else if (option == DAEMONINTERVAL && strstr(currConf, "interval")) {
                        break;
                }
        }

        /* get file size and allocate memory for it */
        fstat(fileno(f), &buf);
        fileSize = (char *)calloc((size_t)buf.st_size, sizeof(char));
        fread(fileSize, sizeof(char), (size_t)buf.st_size, f);

        fsetpos(f, &fpos);
        fputs(newConf, f);
        fputs(fileSize, f);

        fclose(f);
        free(fileSize);
        strcpy(newConf, "");

        return 0;
}

int readVal(int *currVal) {
        char readCfg[MAXLENSENTENCE];

        FILE *f = fopen(CONFFILE, "r");

        if (!f) {
                return 1;
        }
        while (!feof(f)) {
                fgets(readCfg, 128, f);

                if (strchr(readCfg, '#')) {
                        continue;
                } else if (strstr(readCfg, "undervolt 0 \'CPU\'")) {
                        currVal[CPUOFFSET] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 1 \'GPU\'")) {
                        currVal[GPUOFFSET] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 2 \'CPU Cache\' ")) {
                        currVal[CPUCACHEOFFSET] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 3 \'System Agent\'")) {
                        currVal[SYSAOFFSET] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 4 \'Analog I/O\'")) {
                        currVal[ANALOGIOOFFSET] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "interval")) {
                        currVal[DAEMONINTERVAL] = (int)strtof(strrchr(readCfg, ' ')+1, NULL);
                }
        }

        return 0;
}

short applyValues(int *newValues, int *currentValues) {
        FILE *fproc;
        /* count how much values have changed
         * if zero, non has changed and nothing has been set
         * if -1, error while applying config
         */
        short count = 0;

        if (newValues[CPUOFFSET] != currentValues[CPUOFFSET]) {
                if (setValIntoConfFile(newValues[CPUOFFSET], CPUOFFSET))
                        return -1;
                count++;
                currentValues[CPUOFFSET] = newValues[CPUOFFSET];
        }
        if (newValues[GPUOFFSET] != currentValues[GPUOFFSET]) {
                if (setValIntoConfFile(newValues[GPUOFFSET], GPUOFFSET))
                        return -1;
                count++;
                currentValues[GPUOFFSET] = newValues[GPUOFFSET];
        }
        if (newValues[CPUCACHEOFFSET] != currentValues[CPUCACHEOFFSET]) {
                if (setValIntoConfFile(newValues[CPUCACHEOFFSET], CPUCACHEOFFSET))
                        return -1;
                count++;
                currentValues[CPUCACHEOFFSET] = newValues[CPUCACHEOFFSET];
        }
        if (newValues[SYSAOFFSET] != currentValues[SYSAOFFSET]) {
                if (setValIntoConfFile(newValues[SYSAOFFSET], SYSAOFFSET))
                        return -1;
                count++;
                currentValues[SYSAOFFSET] = newValues[SYSAOFFSET];
        }
        if (newValues[ANALOGIOOFFSET] != currentValues[ANALOGIOOFFSET]) {
                if (setValIntoConfFile(newValues[ANALOGIOOFFSET], ANALOGIOOFFSET))
                        return -1;
                count++;
                currentValues[ANALOGIOOFFSET] = newValues[ANALOGIOOFFSET];
        }
        if (newValues[DAEMONINTERVAL] != currentValues[DAEMONINTERVAL]) {
                if (setValIntoConfFile(newValues[DAEMONINTERVAL], DAEMONINTERVAL))
                        return -1;
                count++;
                currentValues[DAEMONINTERVAL] = newValues[DAEMONINTERVAL];
        }

        /* write to file and apply settings */
        if ((fproc = popen("intel-undervolt apply", "r")) == NULL) {
                count = -1;
        }

        pclose(fproc);
        return count;
}

void *measurePowAndTemp (void *args) {
        int maxname = 0;
        powerMeasureThread *sarg = args;

        powercap_list_t *pcapList = getPowercap(&maxname);
        hwmon_list_t *hwmonList = getCoretemp(&maxname);

        while ( !measurePowerConsumption(sarg->currPowerVals, pcapList, POWERVALEL, maxname)
                && !getCpuTemp(sarg->currTempVals, hwmonList, TEMPERATUREEL, &maxname) ) {
                        sleep(1);
        }

        while (pcapList) {
		powercap_list_t *next = pcapList->next;
		free(pcapList->name);
		free(pcapList->dir);
		free(pcapList);
		pcapList = next;
	}

        while (hwmonList) {
		hwmon_list_t *next = hwmonList->next;
		free(hwmonList->name);
		free(hwmonList->dir);
		free(hwmonList);
		hwmonList = next;
        }

        return ((void *)NULL);
}

int measurePowerConsumption(char currPVals[][BUFSZSMALL], powercap_list_t *pcapList, int elCount, int maxname) {
        int i = 0;
        powercap_list_t *pListNext = pcapList;

        savePowercapNextAsDouble(pcapList, maxname);

        while (pListNext) {
                if (i < elCount) {
                        snprintf(currPVals[i], BUFSZSMALL, "%s : %3.03f W", pListNext->name, pListNext->val);
                        ++i;
                } else {
                        break;
                }
                pListNext = pListNext->next;
        }

        return 0;
}

int getCpuTemp(char currTempVals[][BUFSZSMALL], hwmon_list_t *hwlst, int elCount, int *maxname){
        char tmp[BUFSZSMALL];
        int i = 0;
        hwlst = getCoretemp(maxname);
        if (hwlst != NULL) {
                getHwmonNextValues(hwlst, *maxname, tmp);
                hwmon_list_t *hwmonNext = hwlst;
                while (hwmonNext && i < elCount) {
                        snprintf(currTempVals[i], BUFSZSMALL, "%.80s %.80s", hwmonNext->name, hwmonNext->val);
                        hwmonNext = hwmonNext->next;
                        ++i;
                }
        }
        return 0;
}

int powerLimitAlt(float shortPowerVal, float longPowerVal) {
        /* TODO */
        return 0;
}

int tempOffsetAlt(int temp) {
        /* TODO */
        return 0;
}

int daemonUpdateInterval(unsigned int time) {
        return setValIntoConfFile(time, DAEMONINTERVAL);
}

int systemdService(int set) {
        FILE *f;
        char tmpTxt[BUFFERSIZE];

        if (set) {
                if ((f = popen("sudo systemctl enable intel-undervolt", "r")) == NULL) {
                        return -1;
                }
                pclose(f);
                if ((f = popen("sudo systemctl start intel-undervolt", "r")) == NULL) {
                        return -1;
                }
        } else if (set == 0) {
                if ((f = popen("sudo systemctl disable intel-undervolt", "r")) == NULL) {
                        return -1;
                }
                pclose(f);
                if ((f = popen("sudo systemctl stop intel-undervolt", "r")) == NULL) {
                        return -1;
                }
        } else {
                if ((f = popen("sudo systemctl status intel-undervolt", "r")) == NULL) {
                        return -1;
                }
                while (fgets(tmpTxt, BUFFERSIZE, f) != NULL) {
                        if (strstr(tmpTxt, "intel-undervolt.service; enabled") != NULL) {
                                pclose(f);
                                return 0;
                        }
                }
        }
        pclose(f);

        if ((f = popen("sudo systemctl status intel-undervolt", "r")) == NULL) {
                return -1;
        }
        while (fgets(tmpTxt, BUFFERSIZE, f) != NULL) {
                if (strstr(tmpTxt, "status=0/SUCCESS")) {
                        pclose(f);
                        return 0;
                } else if (strstr(tmpTxt, "intel-undervolt.service; disabled") != NULL) {
                        pclose(f);
                        return 0;
                }
        }
        pclose(f);
        return -1;
}

int uvResetAll() {
        char *resetValues = "# CPU Undervolting\n"
                        "# Usage: undervolt ${index} ${display_name} ${undervolt_value}\n"
                        "# Example: undervolt 2 'CPU Cache' -25.84\n\n"
                        "undervolt 0 'CPU' 0\n"
                        "undervolt 1 'GPU' 0\n"
                        "undervolt 2 'CPU Cache' 0\n"
                        "undervolt 3 'System Agent' 0\n"
                        "undervolt 4 'Analog I/O' 0\n\n"
                        "# Power Limits Alteration\n"
                        "# Usage: power ${domain} ${short_power_value} ${long_power_value}\n"
                        "# Power value: ${power}[/${time_window}][:enabled][:disabled]\n"
                        "# Supported domains: package\n"
                        "# Example: power package 45 35"
                        "# Example: power package 45/0.002 35/28\n"
                        "# Example: power package 45/0.002:disabled 35/28:enabled\n\n"
                        "# Critical Temperature Offset Alteration\n"
                        "# Usage: tjoffset ${temperature_offset}\n"
                        "# Example: tjoffset -20\n\n"
                        "# Daemon Update Interval\n"
                        "# Usage: interval ${interval_in_milliseconds}\n\n"
                        "interval 5000\n";
        FILE *f = fopen(CONFFILE, "w");

        if (!f) {
                return -1;
        }
        if (fprintf(f, "%s", resetValues) < 0) {
                fclose(f);
                return -1;
        }
        fclose(f);

        /* write to file and apply settings */
        if((f = popen("intel-undervolt apply", "r")) == NULL) {
                return -1;
        }

        pclose(f);

        return 0;
}

void intToString(int offset, char *tmp, char *dest) {
        snprintf(tmp, MAXDIGIT, "%d", offset);
        strncat(dest, tmp, UV_OPTION);
}
