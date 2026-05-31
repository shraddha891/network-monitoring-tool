# Distributed Network Monitoring Tool (C++ / Linux)

A lightweight command-line tool that monitors multiple network nodes over TCP/IP, measures latency, detects packet loss, and raises alerts.

## Features
- Probes multiple nodes concurrently using TCP connect timing
- Detects node DOWN / HIGH LATENCY / PACKET LOSS conditions
- Alert system with timestamps
- Summary report after monitoring cycles
- Configurable nodes (IP + port)

## Build & Run
```bash
make
./netmon
```

> Note: Some IPs (e.g. 192.168.x.x) will show DOWN if not on your LAN — that's expected and demonstrates the alert system.

## Project Structure
```
src/
  main.c    — node registration, monitor lifecycle
  netmon.c  — probe, alert, report logic
  netmon.h  — types and API
Makefile
README.md
```

## Concepts Demonstrated
- TCP socket programming (`connect` with timeout)
- IP protocol handling (`inet_pton`, `sockaddr_in`)
- Latency measurement using `gettimeofday`
- Alert threshold logic and diagnostic reporting
- Distributed system monitoring patterns
