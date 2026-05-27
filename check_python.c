#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

    int ret = system("which python3.9 > /dev/null 2>&1");

    if (ret != 0) {
        fprintf(stderr, "Error: Python 3.9 not found\n");
        return 1;
    }

    FILE *fp = popen("python3.9 --version 2>&1", "r");

    if (!fp) {
        perror("popen failed");
        return 1;
    }

    char version[128] = {0};

    fgets(version, sizeof(version), fp);

    pclose(fp);

    version[strcspn(version, "\n")] = 0;

    printf("Detected Python Version: %s\n", version);

    return 0;
}
