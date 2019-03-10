#include "measure.h"

void savePowercapNextAsDouble(powercap_list_t *lst, int maxname) {
	char buf[BUFSZSMALL];
	if (!lst) {
		return;
	}

	savePowercapNextAsDouble(lst->next, maxname);

	sprintf(buf, DIR_POWERCAP "/%s/energy_uj", lst->dir);
	int fd = open(buf, O_RDONLY);
	if (fd >= 0) {
		int count = read(fd, buf, BUFSZSMALL - 1);
		if (count > 0) {
			buf[buf[count - 1] == '\n' ? count - 1 : count] = '\0';
			int64_t val = (int64_t) atoll(buf);
			struct timespec tnow;
			clock_gettime(CLOCK_MONOTONIC, &tnow);
			if (lst->last > 0) {
				struct timespec tdiff;
				tdiff.tv_sec = tnow.tv_sec - lst->time.tv_sec;
				tdiff.tv_nsec = tnow.tv_nsec - lst->time.tv_nsec;
				while (tdiff.tv_nsec < 0) {
					tdiff.tv_nsec += 1000000000;
					tdiff.tv_sec--;
				}
				int64_t diff = (int64_t) (tdiff.tv_sec * 1000000000 + tdiff.tv_nsec);
				lst->val = (double) (val - lst->last) * 1000 / diff;
			}
			lst->last = val;
			lst->time = tnow;
		}
		close(fd);
	}
}

powercap_list_t *getPowercap(int *maxname) {
	char buf[BUFSZSMALL];
	powercap_list_t * lst = NULL;

	DIR * dir = opendir(DIR_POWERCAP);
	if (dir == NULL) {
		fprintf(stderr, "Failed to open powercap directory\n");
		return NULL;
	}
	struct dirent * dirent;
	while ((dirent = readdir(dir))) {
		if (strstr(dirent->d_name, ":") && strlen(dirent->d_name) <= 30) {
			snprintf(buf, BUFSZSMALL, DIR_POWERCAP "/%s/name", dirent->d_name);
			int fd = open(buf, O_RDONLY);
			if (fd >= 0) {
				int count = read(fd, buf, BUFSZSMALL - 1);
				if (count > 1) {
					int nlen = buf[count - 1] == '\n' ? count - 2 : count - 1;
					buf[nlen + 1] = '\0';
					powercap_list_t * nlst = malloc(sizeof(powercap_list_t));
					nlst->next = lst;
					lst = nlst;
					nlst->name = malloc(nlen + 1);
					memcpy(nlst->name, buf, nlen + 1);
					int dlen = strlen(dirent->d_name);
					nlst->dir = malloc(dlen + 1);
					memcpy(nlst->dir, dirent->d_name, dlen + 1);
					nlst->last = 0;
					if (maxname) {
						*maxname = nlen > *maxname ? nlen : *maxname;
					}
				}
				close(fd);
			}
		}
	}
	closedir(dir);

	return lst;
}

void getHwmonNextValues(hwmon_list_t *lst, int maxname, char *buf) {
	if (!lst) {
		return;
	}

	getHwmonNextValues(lst->next, maxname, buf);

	sprintf(buf, DIR_HWMON "/%s/temp%d_input", lst->dir, lst->index);
	int fd = open(buf, O_RDONLY);
	if (fd >= 0) {
		int count = read(fd, buf, BUFSZSMALL - 1);
		if (count > 0) {
			buf[buf[count - 1] == '\n' ? count - 1 : count] = '\0';
			double dval = atol(buf) / 1000.;
			snprintf(lst->val, BUFSZSMALL, "%6.01f%s", dval, " C");
		}
		close(fd);
	}
}

bool getHwmon(const char * name, char * out) {
	char buf[BUFSZSMALL];

	DIR * dir = opendir(DIR_HWMON);
	if (dir == NULL) {
		fprintf(stderr, "Failed to open hwmon directory\n");
		return NULL;
	}
	struct dirent * dirent;
	while ((dirent = readdir(dir))) {
		if (strlen(dirent->d_name) <= 30) {
			snprintf(buf, BUFSZSMALL, DIR_HWMON "/%s/name", dirent->d_name);
			int fd = open(buf, O_RDONLY);
			if (fd >= 0) {
				int count = read(fd, buf, BUFSZSMALL - 1);
				if (count > 1) {
					int nlen = buf[count - 1] == '\n' ? count - 2 : count - 1;
					buf[nlen + 1] = '\0';
					if (!strcmp(buf, name)) {
						strcpy(out, dirent->d_name);
						close(fd);
						closedir(dir);
						return true;
					}
				}
				close(fd);
			}
		}
	}
	closedir(dir);

	return false;
}

hwmon_list_t *getCoretemp(int *maxname) {
	char hdir[BUFSZSMALL];
	char buf[BUFSZSMALL];
	hwmon_list_t * lst = NULL;

	if (!getHwmon("coretemp", hdir)) {
		fprintf(stderr, "Failed to find coretemp hwmon\n");
		return NULL;
	}

	int i;
	for (i = 1;; i++) {
		snprintf(buf, BUFSZSMALL, DIR_HWMON "/%.256s/temp%d_input", hdir, i);
		int fd = open(buf, O_RDONLY);
		if (fd < 0) {
			break;
		}
		snprintf(buf, BUFSZSMALL, DIR_HWMON "/%.256s/temp%d_label", hdir, i);
		fd = open(buf, O_RDONLY);
		char * name = NULL;
		if (fd >= 0) {
			int count = read(fd, buf, BUFSZSMALL - 1);
			if (count > 1) {
				int nlen = buf[count - 1] == '\n' ? count - 2 : count - 1;
				buf[nlen + 1] = '\0';
				name = malloc(strlen(buf) + 1);
				strcpy(name, buf);
			}
			close(fd);
		}
		if (!name) {
			sprintf(buf, "temp%d", i);
			name = malloc(strlen(buf) + 1);
			strcpy(name, buf);
		}
		hwmon_list_t * nlst = malloc(sizeof(hwmon_list_t));
		nlst->next = lst;
		lst = nlst;
		nlst->name = name;
		strncat(nlst->name, " : ", 4);
		nlst->dir = malloc(strlen(hdir) + 1);
		strcpy(nlst->dir, hdir);
		nlst->index = i;
		if (maxname) {
			int len = strlen(name);
			*maxname = len > *maxname ? len : *maxname;
		}
	}

	return lst;
}

/*
void printCpufreq(int maxname, char *buf, bool *nl) {
	bool nll = false;

	int i;
	for (i = 0;; i++) {
		sprintf(buf, "/sys/bus/cpu/devices/cpu%d/cpufreq/scaling_cur_freq", i);
		int fd = open(buf, O_RDONLY);
		if (fd < 0) {
			break;
		}
		int count = read(fd, buf, BUFSZSMALL - 1);
		if (count > 0) {
			buf[buf[count - 1] == '\n' ? count - 1 : count] = '\0';
			double dval = atol(buf) / 1000.;
			sprintf(buf, "Core %d", i);
			printf("%9.03f MHz\n", dval);
		}
		close(fd);
	}
}
*/
