import re
import subprocess
import sys
import time

nWriters = [10, 25, 50, 75, 100]

def runBatch(testPrefix):
    moduleName = '%s_simple' % testPrefix

    rmmodCommand = 'rmmod %s_simple' % testPrefix
    lsmodCommand = 'lsmod | grep %s' % moduleName
    for w in nWriters:
        baseCommand = 'insmod modules/%s.ko nReaders=100 nWriters=%d readerDelay=0 writerDelay=0' % (moduleName, w)
        fileName = 'data/kernel/%s-%d.txt' % (testPrefix, w)
        for i in range(100):
            command = '%s testNumber=%d' % (baseCommand, i)
            subprocess.check_call(command, shell=True)
            print command
            time.sleep(4)
            print "Will call now rmmod"
            subprocess.check_call(rmmodCommand, shell=True)

            try:
                lsmodOut = subprocess.check_output(lsmodCommand, shell=True)
                while lsmodOut != '':
                    print 'Module not removed yet: %s' % lsmodOut
                    time.sleep(1)
                    lsmodOut = subprocess.check_output(lsmodCommand, shell=True)
            except subprocess.CalledProcessError:
                print 'Great! Module unloaded'

            searchString = '\[PP FINAL\] Test: %d, nr_reads: \d+, nr_writes: \d+' % i
            dmesgOut = subprocess.check_output('dmesg | tail -10', shell=True)
            result = re.search(searchString, dmesgOut)
            f = open(fileName, 'a')
            f.write('%s\n' % result.group(0))
            f.close()
            time.sleep(1)

if __name__ == '__main__':
    runBatch('rcu')
    runBatch('rwlock')
