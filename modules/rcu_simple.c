#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thiago de Baros Lacerda and Andre Guedes Linhares");
MODULE_DESCRIPTION("Simple RCU pointer read/update benchmark");

uint nReaders = 0;
uint readerDelay = 0;
uint nWriters = 0;
uint writerDelay = 0;
uint duration = 0;

struct task_struct **readers;
ulong* readCount;
struct task_struct **writers;
ulong* writeCount;
spinlock_t writerLock;
ulong totalRead = 0;
ulong totalWrite = 0;

char* baseThreadName;

struct timer_list testTimer;

int cleaningInTimer = 0;
int threadsClean = 0;

module_param(nReaders, uint, 0);
MODULE_PARM_DESC(nReaders, "Number of reader threads.");
module_param(readerDelay, uint, 0);
MODULE_PARM_DESC(readerDelay, "Amount of time a reader thread should sleep between reads (in ms).");
module_param(nWriters, uint, 0);
MODULE_PARM_DESC(nWriters, "Number of writer threads.");
module_param(writerDelay, uint, 0);
MODULE_PARM_DESC(writerDelay, "Amount of time a writer thread should sleep between reads (in ms).");
module_param(duration, uint, 0);
MODULE_PARM_DESC(duration, "Duration of the test in ms.");

typedef struct testRCUStruct {
    int a;
} TestRCUStruct;

TestRCUStruct *foo;

int writerRoutine(void* data) {
    int index = *(int*) data;
    TestRCUStruct *newStruct;
    TestRCUStruct *oldStruct;
    while(!kthread_should_stop()) {
        newStruct = kmalloc(sizeof(TestRCUStruct), GFP_KERNEL);
        spin_lock(&writerLock);
        oldStruct = foo;
        newStruct->a = oldStruct->a;
        rcu_assign_pointer(foo, newStruct);
        spin_unlock(&writerLock);
        synchronize_rcu();
        writeCount[index]++;
        kfree(oldStruct);
        msleep(writerDelay);
    }
    return 0;
}

int readerRoutine(void* data) {
    int index = *(int*) data;
    int retval;
    while(!kthread_should_stop()) {
        rcu_read_lock();
        retval = rcu_dereference(foo)->a;
        if (retval != 8) {
            printk(KERN_ERR "Error: Inconsistent value of struct\n");
            do_exit(-1);
        }
        readCount[index]++;
        rcu_read_unlock();
        msleep(readerDelay);
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
    printk(KERN_INFO "[PP] Total reads: %lu, total writes: %lu\n", totalRead, totalWrite);
    threadsClean = 0;
}

void endOfTest(ulong data) {
    del_timer(&testTimer);
    printk(KERN_INFO "[PP] Time is up!\n");
    cleaningInTimer = 1;
    cleanThreads();
    cleaningInTimer = 0;
}

static int __init rcuSimpleInit(void) {
    int i;
    int ret;
    if (nReaders == 0 || nWriters == 0 || duration == 0) {
        printk(KERN_ERR "Error: nReaders, nWriters and duration must be positive integers\n");
        return -EPERM;
    }

    setup_timer(&testTimer, endOfTest, 0);
    spin_lock_init(&writerLock);
    foo = kmalloc(sizeof(TestRCUStruct), GFP_KERNEL);
    foo->a = 8;
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
    ret = mod_timer(&testTimer, jiffies + msecs_to_jiffies(duration));
    if (ret)
        printk("Error in mod_timer\n");

    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit rcuSimpleCleanup(void) {
    if (cleaningInTimer)
        while(cleaningInTimer);
    else if (!threadsClean)
        cleanThreads();

    printk(KERN_INFO "[PP] Cleaning up module.\n");
}

module_init(rcuSimpleInit);
module_exit(rcuSimpleCleanup);

