#include "intel_undervolt.h"

int setValIntoConfFile(int offset, short option){
        char newConf[MAXLENSENTENCE];
        char tmp[MAXLENSENTENCE];
        char currConf[MAXLENSENTENCE];
        char *fileSize;
        struct stat *buf; 

        fpos_t fpos;

        FILE *f = fopen(CONFFILE, "r+");

        if (!f) {
                return 1;
        }

        switch (option)
        {
                case CPUOFFSET:
                        strncat(newConf, "undervolt 0 'CPU' ", 19);
                        intToString(offset, tmp, newConf);
                        break;
                case GPUOFFSET:
                        strncat(newConf, "undervolt 1 'GPU' ", 19);
                        intToString(offset, tmp, newConf);
                        break;
                case CPUCACHEOFFSET:
                        strncat(newConf, "undervolt 2 'CPU Cache' ", 25);
                        intToString(offset, tmp, newConf);
                        break;
                case SYSAOFFSET:
                        strncat(newConf, "undervolt 3 'System Agent' ", 28);
                        intToString(offset, tmp, newConf);
                        break;
                case ANALOGIOOFFSET:
                        strncat(newConf, "undervolt 4 'Analog I/O' ", 26);
                        intToString(offset, tmp, newConf);
                        break;
                default:
                        return 1;
        }

        while (!feof(f)) {
                fgetpos(f, &fpos);
                fgets(currConf, MAXLENSENTENCE, f);

                if (strchr(currConf, '#') || strlen(currConf) < 8){
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
                }
        }
        strncat(newConf, "\n", 2);

        /* get file size and allocate memory for it */
        fstat(fileno(f), buf);
        fileSize = (char *)calloc((size_t)buf->st_size, sizeof(char));
        fread(fileSize, sizeof(char), (size_t)buf->st_size, f);

        fsetpos(f, &fpos);
        fputs(newConf, f);
        fputs(fileSize, f);

        fclose(f);
        free(fileSize);
        strcpy(newConf, "");

        return 0;
}

int readVal(int *currVal){
        char readCfg[MAXLENSENTENCE];

        FILE *f = fopen(CONFFILE, "r");

        if (!f) {
                return 1;
        }
        while (!feof(f)) {
                fgets(readCfg, 128, f);

                if (strchr(readCfg, '#')){
                        continue;
                } else if (strstr(readCfg, "undervolt 0 \'CPU\'")) {
                        currVal[CPUOFFSET] = strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 1 \'GPU\'")) {
                        currVal[GPUOFFSET] = strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 2 \'CPU Cache\' ")) {
                        currVal[CPUCACHEOFFSET] = strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 3 \'System Agent\'")) {
                        currVal[SYSAOFFSET] = strtof(strrchr(readCfg, ' ')+1, NULL);
                } else if (strstr(readCfg, "undervolt 4 \'Analog I/O\'")) {
                        currVal[ANALOGIOOFFSET] = strtof(strrchr(readCfg, ' ')+1, NULL);
                }
        }

        return 0;
}

short applyValues(int *newValues, int *currentValues){
        FILE *fproc;
        /* count how much values have changed
         * if zero, non has changed and nothing has been set
         * if -1, error while applying config
         */
        short count = 0;

        if (newValues[CPUOFFSET] != currentValues[CPUOFFSET]){
                if(setValIntoConfFile(newValues[CPUOFFSET], CPUOFFSET))
                        return -1;
                count++;
                currentValues[CPUOFFSET] = newValues[CPUOFFSET];
        }
        if (newValues[GPUOFFSET] != currentValues[GPUOFFSET]){
                if(setValIntoConfFile(newValues[GPUOFFSET], GPUOFFSET))
                        return -1;
                count++;
                currentValues[GPUOFFSET] = newValues[GPUOFFSET];
        }
        if (newValues[CPUCACHEOFFSET] != currentValues[CPUCACHEOFFSET]){
                if(setValIntoConfFile(newValues[CPUCACHEOFFSET], CPUCACHEOFFSET))
                        return -1;
                count++;
                currentValues[CPUCACHEOFFSET] = newValues[CPUCACHEOFFSET];
        }
        if (newValues[SYSAOFFSET] != currentValues[SYSAOFFSET]){
                if(setValIntoConfFile(newValues[SYSAOFFSET], SYSAOFFSET))
                        return -1;
                count++;
                currentValues[SYSAOFFSET] = newValues[SYSAOFFSET];
        }
        if (newValues[ANALOGIOOFFSET] != currentValues[ANALOGIOOFFSET]){
                if(setValIntoConfFile(newValues[ANALOGIOOFFSET], ANALOGIOOFFSET))
                        return -1;
                count++;
                currentValues[ANALOGIOOFFSET] = newValues[ANALOGIOOFFSET];
        }

        /* write to file and apply settings */
        if((fproc = popen("intel-undervolt apply", "r")) == NULL){
                count = -1;
        }

        pclose(fproc);
        return count;
}

void measurePowerConsumption(){
        /* TODO */
        return;
}

int daemonMode(){
        /* TODO */
        return 0;
}

int powerLimitAlt(float shortPowerVal, float longPowerVal){
        /* TODO */
        return 0;
}

int tempOffsetAlt(int temp){
        /* TODO */
        return 0;
}

int daemonUpdateInterval(unsigned int time){
        /* TODO */
        return 0;
}

int uvResetAll(){
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
                return 1;
        }
        if (fprintf(f, "%s", resetValues) < 0) {
                fclose(f);
                return 1;
        }
        fclose(f);
        return 0;
}

void intToString(int offset, char *tmp, char *dest){
        snprintf(tmp, MAXDIGIT, "%d", offset);
        strncat(dest, tmp, UV_OPTION);
}
