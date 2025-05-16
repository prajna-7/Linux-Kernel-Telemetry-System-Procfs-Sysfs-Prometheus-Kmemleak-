#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#define PROC_NAME "telemetry"
#define HISTORY_SIZE 10

struct telemetry_snapshot {
    unsigned long jiffies;
    unsigned long context_switches;
    unsigned long irq_count;
    unsigned long free_ram;
    unsigned long total_ram;
};

static struct telemetry_snapshot history[HISTORY_SIZE];
static int head = 0;
static int count = 0;

static struct kobject *telemetry_kobj;

static char *leak_ptr = NULL;

static void simulate_memory_leak(void) {
    if (!leak_ptr) {
        leak_ptr = kmalloc(1024, GFP_KERNEL);
        if (leak_ptr)
            memset(leak_ptr, 0xAB, 1024);  // Fill to make it visible
        printk(KERN_INFO "Telemetry: Simulated memory leak created\n");
    }
}

static int telemetry_enabled = 1;
static int log_interval = 5;
static unsigned long irq_counter = 0;

static irqreturn_t fake_irq_handler(int irq, void *dev_id) {
    irq_counter++;
    return IRQ_HANDLED;
}

static void save_snapshot(void) {
    struct sysinfo i;
    si_meminfo(&i);

    history[head].jiffies = jiffies;
    history[head].context_switches = nr_context_switches();
    history[head].irq_count = irq_counter;
    history[head].free_ram = i.freeram * i.mem_unit / 1024 / 1024;
    history[head].total_ram = i.totalram * i.mem_unit / 1024 / 1024;

    head = (head + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE)
        count++;
}

static int telemetry_show(struct seq_file *m, void *v) {
    int i;
    int index;

    if (!telemetry_enabled) {
        seq_printf(m, "{\n  \"error\": \"Telemetry disabled\"\n}\n");
        return 0;
    }

    save_snapshot();
    seq_printf(m, "[\n");
    for (i = 0; i < count; i++) {
        index = (head + i) % HISTORY_SIZE;
        seq_printf(m, "  {\n");
        seq_printf(m, "    \"uptime_jiffies\": %lu,\n", history[index].jiffies);
        seq_printf(m, "    \"context_switches\": %lu,\n", history[index].context_switches);
        seq_printf(m, "    \"irq_count\": %lu,\n", history[index].irq_count);
        seq_printf(m, "    \"free_ram_MB\": %lu,\n", history[index].free_ram);
        seq_printf(m, "    \"total_ram_MB\": %lu\n", history[index].total_ram);
        seq_printf(m, "  }%s\n", (i < count - 1) ? "," : "");
    }
    seq_printf(m, "]\n");
    return 0;
}

static int telemetry_open(struct inode *inode, struct file *file) {
    return single_open(file, telemetry_show, NULL);
}

static const struct proc_ops telemetry_fops = {
    .proc_open = telemetry_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", telemetry_enabled);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%du", &telemetry_enabled);
    return count;
}

static ssize_t interval_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", log_interval);
}

static ssize_t interval_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%du", &log_interval);
    return count;
}

static struct kobj_attribute enable_attr = __ATTR(enable, 0660, enable_show, enable_store);
static struct kobj_attribute interval_attr = __ATTR(log_interval, 0660, interval_show, interval_store);

static int __init telemetry_init(void) {
    simulate_memory_leak();
    int ret;

    proc_create(PROC_NAME, 0, NULL, &telemetry_fops);

    telemetry_kobj = kobject_create_and_add("telemetry", kernel_kobj);
    if (!telemetry_kobj)
        return -ENOMEM;

    sysfs_create_file(telemetry_kobj, &enable_attr.attr);
    sysfs_create_file(telemetry_kobj, &interval_attr.attr);

    ret = request_irq(1, fake_irq_handler, IRQF_SHARED, "fake_irq", (void *)(fake_irq_handler));
    if (ret) {
        printk(KERN_ERR "Failed to request IRQ 1\n");
    }

    printk(KERN_INFO "Telemetry module with circular buffer loaded\n");
    return 0;
}

static void __exit telemetry_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    sysfs_remove_file(telemetry_kobj, &enable_attr.attr);
    sysfs_remove_file(telemetry_kobj, &interval_attr.attr);
    kobject_put(telemetry_kobj);
    free_irq(1, (void *)(fake_irq_handler));
    printk(KERN_INFO "Telemetry module removed\n");
}

module_init(telemetry_init);
module_exit(telemetry_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Telemetry Module with Circular Buffer");
MODULE_VERSION("4.0");
