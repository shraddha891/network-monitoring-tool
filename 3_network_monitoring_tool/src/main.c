#include "netmon.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("=== Distributed Network Monitoring Tool ===\n\n");

    Monitor *m = monitor_create();
    if (!m) { fprintf(stderr, "Failed to create monitor\n"); return 1; }

    /* Register nodes to monitor (IP:port) */
    monitor_add_node(m, "Gateway",    "192.168.1.1",   80);
    monitor_add_node(m, "Server-1",   "192.168.1.10",  8080);
    monitor_add_node(m, "Server-2",   "192.168.1.11",  8080);
    monitor_add_node(m, "DNS",        "8.8.8.8",       53);
    monitor_add_node(m, "WebService", "93.184.216.34", 80);  /* example.com */

    /* Run 3 monitoring cycles */
    monitor_run(m, 3);

    /* Print summary */
    monitor_report(m);
    monitor_destroy(m);

    return EXIT_SUCCESS;
}
