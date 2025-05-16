#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define TELEMETRY_FILE "/proc/telemetry"
#define PORT 9090
#define MAX 8192

void export_metrics(int client_fd) {
    FILE *fp = fopen(TELEMETRY_FILE, "r");
    if (!fp) {
        dprintf(client_fd, "# HELP telemetry_error Error reading telemetry\n");
        dprintf(client_fd, "# TYPE telemetry_error counter\n");
        dprintf(client_fd, "telemetry_error 1\n");
        return;
    }

    char buffer[MAX];
    int context_switches = 0, irq_count = 0, free_ram = 0, total_ram = 0;

    while (fgets(buffer, sizeof(buffer), fp)) {
        sscanf(buffer, "    \"context_switches\": %d,", &context_switches);
        sscanf(buffer, "    \"irq_count\": %d,", &irq_count);
        sscanf(buffer, "    \"free_ram_MB\": %d,", &free_ram);
        sscanf(buffer, "    \"total_ram_MB\": %d", &total_ram);
    }
    fclose(fp);

    dprintf(client_fd, "# HELP telemetry_context_switches Total context switches\n");
    dprintf(client_fd, "# TYPE telemetry_context_switches counter\n");
    dprintf(client_fd, "telemetry_context_switches %d\n", context_switches);
    dprintf(client_fd, "# HELP telemetry_irq_count Total IRQs handled\n");
    dprintf(client_fd, "# TYPE telemetry_irq_count counter\n");
    dprintf(client_fd, "telemetry_irq_count %d\n", irq_count);
    dprintf(client_fd, "# HELP telemetry_free_ram_MB Free RAM in MB\n");
    dprintf(client_fd, "# TYPE telemetry_free_ram_MB gauge\n");
    dprintf(client_fd, "telemetry_free_ram_MB %d\n", free_ram);
    dprintf(client_fd, "# HELP telemetry_total_ram_MB Total RAM in MB\n");
    dprintf(client_fd, "# TYPE telemetry_total_ram_MB gauge\n");
    dprintf(client_fd, "telemetry_total_ram_MB %d\n", total_ram);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server, client;
    socklen_t len = sizeof(client);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) != 0) {
        perror("Socket bind failed");
        exit(1);
    }

    if (listen(server_fd, 5) != 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Prometheus exporter running on port %d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client, &len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        export_metrics(client_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
