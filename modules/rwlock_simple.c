#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thiago de Baros Lacerda and Andre Guedes Linhares");
MODULE_DESCRIPTION("Simple rwlock pointer read/update benchmark");

uint nReaders = 0;
uint readerDelay = 0;
uint nWriters = 0;
uint writerDelay = 0;
uint testNumber = 0;

struct task_struct *mainTask;
struct task_struct **readers;
ulong* readCount;
struct task_struct **writers;
ulong* writeCount;
rwlock_t readWriteLock;
ulong totalRead = 0;
ulong totalWrite = 0;

char* baseThreadName;

atomic_t threadsCounter = ATOMIC_INIT(0);

module_param(nReaders, uint, 0);
MODULE_PARM_DESC(nReaders, "Number of reader threads.");
module_param(readerDelay, uint, 0);
MODULE_PARM_DESC(readerDelay, "Amount of time a reader thread should sleep between reads (in ms).");
module_param(nWriters, uint, 0);
MODULE_PARM_DESC(nWriters, "Number of writer threads.");
module_param(writerDelay, uint, 0);
MODULE_PARM_DESC(writerDelay, "Amount of time a writer thread should sleep between reads (in ms).");
module_param(testNumber, uint, 0);
MODULE_PARM_DESC(testNumber, "Test nummber.");

typedef struct testStruct {
    int a;
} TestStruct;

TestStruct *foo;

int writerRoutine(void* data) {
    int index = *(int*) data;
    TestStruct *newStruct;
    TestStruct *oldStruct;
    set_current_state(TASK_INTERRUPTIBLE);
    while(!kthread_should_stop()) {
        newStruct = kmalloc(sizeof(TestStruct), GFP_KERNEL);
        write_lock(&readWriteLock);
        oldStruct = foo;
        newStruct->a = oldStruct->a;
        foo = newStruct;
        write_unlock(&readWriteLock);
        writeCount[index]++;
        kfree(oldStruct);
    }
    if (atomic_dec_and_test(&threadsCounter)) {
        printk(KERN_INFO "[PP] Last thread (a writer) will wake main.\n");
        wake_up_process(mainTask);
    }
    return 0;
}

int readerRoutine(void* data) {
    int index = *(int*) data;
    int retval;
    set_current_state(TASK_INTERRUPTIBLE);
    while(!kthread_should_stop()) {
        read_lock(&readWriteLock);
        retval = foo->a;
        if (retval != 8) {
            printk(KERN_ERR "Error: Inconsistent value of struct\n");
            do_exit(-1);
        }
        readCount[index]++;
        read_unlock(&readWriteLock);
    }
    if (atomic_dec_and_test(&threadsCounter)) {
        printk(KERN_INFO "[PP] Last thread (a reader) will wake main.\n");
        wake_up_process(mainTask);
    }
    return 0;
}

void cleanThreads(void) {
    int i;
    printk(KERN_INFO "[PP] Stopping all threads\n");
    for (i = 0; i < nReaders; ++i) {
        kthread_stop(readers[i]);
        totalRead += readCount[i];
    }
    for (i = 0; i < nWriters; ++i) {
        kthread_stop(writers[i]);
        totalWrite += writeCount[i];
    }
    printk(KERN_INFO "[PP FINAL] Test: %u, nr_reads: %lu, nr_writes: %lu\n", testNumber, totalRead, totalWrite);
}

static int __init rwlockSimpleInit(void) {
    int i;
    if (nReaders == 0 || nWriters == 0) {
        printk(KERN_ERR "Error: nReaders and nWriters must be positive integers\n");
        return -EPERM;
    }

    rwlock_init(&readWriteLock);
    foo = kmalloc(sizeof(TestStruct), GFP_KERNEL);
    foo->a = 8;
    atomic_set(&threadsCounter, nReaders + nWriters);
    baseThreadName = kmalloc(sizeof(char) * 5, GFP_KERNEL);
    readers = kmalloc(sizeof(struct task_struct*) * nReaders, GFP_KERNEL);
    readCount = kmalloc(sizeof(ulong) * nReaders, GFP_KERNEL);
    memset(readCount, 0, sizeof(ulong) * nReaders);
    for (i = 0; i < nReaders; ++i) {
        sprintf(baseThreadName, "r_%d", i);
        readers[i] = kthread_run(&readerRoutine, (void*) &i, baseThreadName);
    }
    writers = kmalloc(sizeof(struct task_struct*) * nWriters, GFP_KERNEL);
    writeCount = kmalloc(sizeof(ulong) * nWriters, GFP_KERNEL);
    memset(writeCount, 0, sizeof(ulong) * nWriters);
    for (i = 0; i < nWriters; ++i) {
        sprintf(baseThreadName, "w_%d", i);
        writers[i] = kthread_run(&writerRoutine, (void*) &i, baseThreadName);
    }

    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit rwlockSimpleCleanup(void) {
    printk(KERN_INFO "[PP] Exit routine called.\n");
    mainTask = current;
    set_current_state(TASK_INTERRUPTIBLE);
    cleanThreads();
    printk(KERN_INFO "[PP] Called cleanThreads, will now sleep\n");
    schedule();

    printk(KERN_INFO "[PP] WAKE\n");
    kfree(readers);
    kfree(writers);

    kfree(readCount);
    kfree(writeCount);
    kfree(foo);
    kfree(baseThreadName);

    printk(KERN_INFO "[PP] Cleaning up module.\n");
}

module_init(rwlockSimpleInit);
module_exit(rwlockSimpleCleanup);

