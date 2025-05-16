# Linux Kernel Telemetry System 🧠

A complete Linux kernel module that monitors system performance in real time.  
It exposes metrics through `/proc`, allows dynamic control via `sysfs`, logs data in JSON, exports to Prometheus, and even includes a simulated memory leak to test kernel debugging tools like `kmemleak`.

---

## 🔧 Features

### 🧱 Kernel Module (C):
- `/proc/telemetry`: Shows latest **10 telemetry snapshots** (uptime, memory, IRQs, context switches)
- `Sysfs` control:
  - `enable`: Toggle telemetry on/off  
  - `log_interval`: Adjust log interval dynamically
- **IRQ tracking**: Counts interrupts via a shared IRQ handler
- **Circular buffer**: Stores history in kernel space
- **Simulated memory leak**: Triggered at module load for `kmemleak` testing

### 🖥️ User-Space Tools:
- `telemetry_logger_json.c`: Logs telemetry to `telemetry_log.json`
- `prometheus_exporter.c`: Exposes metrics at `http://localhost:9090/metrics`

---

## 🚀 Setup Instructions

### 1️⃣ Build and Load the Kernel Module
```bash
cd src
make
sudo insmod telemetry.ko
```

### 2️⃣ View or Control via Sysfs
```bash
cat /sys/kernel/telemetry/enable
echo 1 | sudo tee /sys/kernel/telemetry/enable

cat /sys/kernel/telemetry/log_interval
echo 5 | sudo tee /sys/kernel/telemetry/log_interval
```

### 3️⃣ Check Output
```bash
cat /proc/telemetry
```

---

## 🧾 JSON Logger

```bash
cd userspace
gcc telemetry_logger_json.c -o telemetry_logger
./telemetry_logger
```

- Logs output to: `telemetry_log.json`  
- Runs periodically using the configured log interval

---

## 📡 Prometheus Exporter

```bash
gcc prometheus_exporter.c -o prometheus_exporter
./prometheus_exporter
curl http://localhost:9090/metrics
```

Add this to your `prometheus.yml`:
```yaml
scrape_configs:
  - job_name: 'kernel_telemetry'
    static_configs:
      - targets: ['localhost:9090']
```

---

## 🔍 Kmemleak Debugging

The module allocates memory via `kmalloc()` without freeing it to simulate a memory leak.

### Enable and Check for Leaks
```bash
# Mount debugfs
sudo mount -t debugfs none /sys/kernel/debug

# Trigger scan
echo scan | sudo tee /sys/kernel/debug/kmemleak

# Show results
cat /sys/kernel/debug/kmemleak
```

✅ Requires: kernel compiled with `CONFIG_DEBUG_KMEMLEAK=y` and boot param `kmemleak=on`

---

## 🧹 Clean Up
```bash
sudo rmmod telemetry
make clean
```

---

## 📄 License
GPL-2.0
