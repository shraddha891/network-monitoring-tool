#ifndef NETMON_H
#define NETMON_H

#include <stdint.h>
#include <time.h>

#define MAX_NODES       16
#define MAX_ALERTS      64
#define MONITOR_PORT    9090

/* ── Node status ───────────────────────────────────────────── */
typedef enum {
    NODE_UNKNOWN = 0,
    NODE_UP      = 1,
    NODE_DOWN    = 2,
    NODE_WARN    = 3
} NodeStatus;

/* ── Network node ──────────────────────────────────────────── */
typedef struct {
    char        name[32];
    char        ip[16];
    int         port;
    NodeStatus  status;
    double      latency_ms;
    uint32_t    packets_sent;
    uint32_t    packets_recv;
    time_t      last_seen;
} Node;

/* ── Alert ─────────────────────────────────────────────────── */
typedef enum {
    ALERT_DOWN    = 0,
    ALERT_HIGH_LATENCY = 1,
    ALERT_PACKET_LOSS  = 2
} AlertType;

typedef struct {
    AlertType   type;
    char        node_name[32];
    char        message[128];
    time_t      timestamp;
} Alert;

/* ── Monitor context ───────────────────────────────────────── */
typedef struct {
    Node    nodes[MAX_NODES];
    int     node_count;
    Alert   alerts[MAX_ALERTS];
    int     alert_count;
    int     server_fd;
    int     running;
} Monitor;

/* ── API ───────────────────────────────────────────────────── */
Monitor *monitor_create(void);
int      monitor_add_node(Monitor *m, const char *name, const char *ip, int port);
void     monitor_run(Monitor *m, int cycles);
void     monitor_report(const Monitor *m);
void     monitor_destroy(Monitor *m);

/* Internal */
int      probe_node(Node *node);
void     raise_alert(Monitor *m, AlertType type, const Node *node, const char *msg);
void     print_alert(const Alert *a);

#endif /* NETMON_H */
