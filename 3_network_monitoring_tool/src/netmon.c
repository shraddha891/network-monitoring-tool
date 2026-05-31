#include "netmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

/* ─── helpers ──────────────────────────────────────────────────────────── */

static double now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static const char *status_str(NodeStatus s) {
    switch (s) {
        case NODE_UP:   return "UP";
        case NODE_DOWN: return "DOWN";
        case NODE_WARN: return "WARN";
        default:        return "UNKNOWN";
    }
}

static const char *alert_type_str(AlertType t) {
    switch (t) {
        case ALERT_DOWN:        return "NODE_DOWN";
        case ALERT_HIGH_LATENCY:return "HIGH_LATENCY";
        case ALERT_PACKET_LOSS: return "PACKET_LOSS";
        default:                return "UNKNOWN";
    }
}

/* ─── monitor lifecycle ─────────────────────────────────────────────────── */

Monitor *monitor_create(void) {
    Monitor *m = calloc(1, sizeof(Monitor));
    if (!m) return NULL;
    m->running = 1;
    printf("[Monitor] Distributed Network Monitor initialised.\n");
    return m;
}

int monitor_add_node(Monitor *m, const char *name, const char *ip, int port) {
    if (m->node_count >= MAX_NODES) return -1;
    Node *n = &m->nodes[m->node_count++];
    strncpy(n->name, name, sizeof(n->name) - 1);
    strncpy(n->ip,   ip,   sizeof(n->ip)   - 1);
    n->port   = port;
    n->status = NODE_UNKNOWN;
    printf("[Monitor] Registered node '%s' (%s:%d)\n", name, ip, port);
    return 0;
}

/* ─── probe ─────────────────────────────────────────────────────────────── */

int probe_node(Node *node) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    /* 500 ms connect timeout */
    struct timeval tv = { 0, 500000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons((uint16_t)node->port);
    inet_pton(AF_INET, node->ip, &addr.sin_addr);

    double t0  = now_ms();
    int    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    double t1  = now_ms();
    close(sock);

    node->packets_sent++;
    if (ret == 0) {
        node->latency_ms = t1 - t0;
        node->packets_recv++;
        node->last_seen  = time(NULL);
        node->status     = (node->latency_ms > 200.0) ? NODE_WARN : NODE_UP;
    } else {
        node->status = NODE_DOWN;
    }
    return ret;
}

/* ─── alert ──────────────────────────────────────────────────────────────── */

void raise_alert(Monitor *m, AlertType type, const Node *node, const char *msg) {
    if (m->alert_count >= MAX_ALERTS) return;
    Alert *a = &m->alerts[m->alert_count++];
    a->type  = type;
    a->timestamp = time(NULL);
    strncpy(a->node_name, node->name,  sizeof(a->node_name) - 1);
    strncpy(a->message,   msg,         sizeof(a->message)   - 1);
    print_alert(a);
}

void print_alert(const Alert *a) {
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&a->timestamp));
    printf("  ⚠ [ALERT %s] %-15s | %-14s | %s\n",
           ts, a->node_name, alert_type_str(a->type), a->message);
}

/* ─── monitoring loop ────────────────────────────────────────────────────── */

void monitor_run(Monitor *m, int cycles) {
    printf("\n[Monitor] Starting monitoring — %d cycle(s)...\n", cycles);

    for (int c = 0; c < cycles; c++) {
        printf("\n── Cycle %d/%d ──────────────────────────────\n", c + 1, cycles);

        for (int i = 0; i < m->node_count; i++) {
            Node *n = &m->nodes[i];
            probe_node(n);

            printf("  %-12s %-15s status=%-7s latency=%.1fms  loss=%u/%u\n",
                   n->name, n->ip, status_str(n->status),
                   (n->status == NODE_UP || n->status == NODE_WARN) ? n->latency_ms : 0.0,
                   n->packets_sent - n->packets_recv, n->packets_sent);

            /* Raise alerts based on state */
            if (n->status == NODE_DOWN) {
                raise_alert(m, ALERT_DOWN, n, "Node unreachable");
            } else if (n->status == NODE_WARN) {
                char buf[128];
                snprintf(buf, sizeof(buf), "Latency %.1fms > 200ms threshold", n->latency_ms);
                raise_alert(m, ALERT_HIGH_LATENCY, n, buf);
            }

            double loss_pct = n->packets_sent > 0
                ? 100.0 * (n->packets_sent - n->packets_recv) / n->packets_sent : 0.0;
            if (loss_pct > 10.0) {
                char buf[128];
                snprintf(buf, sizeof(buf), "Packet loss %.1f%%", loss_pct);
                raise_alert(m, ALERT_PACKET_LOSS, n, buf);
            }
        }
        sleep(2);
    }
}

/* ─── final report ───────────────────────────────────────────────────────── */

void monitor_report(const Monitor *m) {
    printf("\n════════════════ MONITOR REPORT ════════════════\n");
    printf("Nodes monitored : %d\n", m->node_count);
    printf("Alerts raised   : %d\n\n", m->alert_count);

    for (int i = 0; i < m->node_count; i++) {
        const Node *n = &m->nodes[i];
        double loss_pct = n->packets_sent > 0
            ? 100.0 * (n->packets_sent - n->packets_recv) / n->packets_sent : 0.0;
        printf("  %-12s | %-7s | avg_latency=%.1fms | loss=%.1f%%\n",
               n->name, status_str(n->status), n->latency_ms, loss_pct);
    }
    printf("═══════════════════════════════════════════════\n");
}

void monitor_destroy(Monitor *m) {
    free(m);
    printf("[Monitor] Shutdown complete.\n");
}
