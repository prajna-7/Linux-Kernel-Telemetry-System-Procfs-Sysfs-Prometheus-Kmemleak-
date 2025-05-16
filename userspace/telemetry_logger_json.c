#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define TELEMETRY_FILE "/proc/telemetry"
#define INTERVAL_FILE "/sys/kernel/telemetry/log_interval"
#define LOG_FILE "telemetry_log.json"

int get_interval() {
    FILE *fp = fopen(INTERVAL_FILE, "r");
    int interval = 5;
    if (fp) {
        fscanf(fp, "%d", &interval);
        fclose(fp);
    }
    return interval;
}

void log_telemetry() {
    FILE *fp = fopen(TELEMETRY_FILE, "r");
    FILE *log_fp = fopen(LOG_FILE, "a");
    if (!fp || !log_fp) {
        perror("Failed to open telemetry or log file");
        if (fp) fclose(fp);
        if (log_fp) fclose(log_fp);
        return;
    }

    char buffer[1024];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(stdout, "[%02d:%02d:%02d] Logging telemetry to %s\n", t->tm_hour, t->tm_min, t->tm_sec, LOG_FILE);
    fprintf(log_fp, "{"timestamp": "%04d-%02d-%02d %02d:%02d:%02d", "telemetry": ",
            t->tm_year+1900, t->tm_mon+1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    int started = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (!started && buffer[0] == '[') {
            started = 1;
        }
        fputs(buffer, log_fp);
    }

    if (!started) fprintf(log_fp, "null");
    fprintf(log_fp, "},\n");

    fclose(fp);
    fclose(log_fp);
}

int main() {
    while (1) {
        log_telemetry();
        sleep(get_interval());
    }
    return 0;
}
