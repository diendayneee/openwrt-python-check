#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

    char version[256] = {0};
    FILE *fp;

    /* ===== CASE 1: Python 3.9 tồn tại ===== */
    if (system("which python3.9 > /dev/null 2>&1") == 0) {

        fp = popen("python3.9 --version 2>&1", "r");

        if (!fp) {
            perror("popen");
            return 1;
        }

        fgets(version, sizeof(version), fp);

        pclose(fp);

        /* Xóa ký tự newline */
        version[strcspn(version, "\n")] = 0;

        if (strstr(version, "Python 3.9")) {

            printf("Detected Python Version: %s\n", version);

            FILE *log = fopen("/tmp/python_ver.log", "w");

            if (log) {
                fprintf(log, "%s\n", version);
                fclose(log);
            }

            return 0;
        }
    }

    /* ===== CASE 2: Có python3 nhưng không phải 3.9 ===== */
    if (system("which python3 > /dev/null 2>&1") == 0) {

        fp = popen("python3 --version 2>&1", "r");

        if (!fp) {
            perror("popen");
            return 1;
        }

        fgets(version, sizeof(version), fp);

        pclose(fp);

        version[strcspn(version, "\n")] = 0;

        printf("Detected Python but not 3.9: %s\n", version);

        FILE *log = fopen("/tmp/python_ver.log", "w");

        if (log) {
            fprintf(log, "%s\n", version);
            fclose(log);
        }

        return 2;
    }

    /* ===== CASE 3: Không có Python ===== */
    fprintf(stderr, "Error: Python 3 not found\n");

    FILE *log = fopen("/tmp/python_ver.log", "w");

    if (log) {
        fprintf(log, "Error: Python 3 not found\n");
        fclose(log);
    }

    return 1;
}
