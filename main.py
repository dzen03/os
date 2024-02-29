import io_
from utills import plot, plot_mpstat, plot_cache
from matplotlib import pyplot as plt
import subprocess

tasks_type1 = {'--cpu': (['--cpu-method float32', '--cpu-method int128decimal32'], ),
               '--cache': (['', '--cache-fence', '--cache-level 1', '--cache-level 2', '--cache-level 3'], ),
}

MAX_CPUS_COUNT = 8
TIMEOUT = 30

for dev, methods_ in tasks_type1.items():
    results = dict()
    for method in methods_[0]:
        res = []
        res_mpstat = []
        for cpus in range(1, MAX_CPUS_COUNT+1):
            list_files = subprocess.run(['./stress-ng-helper', dev, f'{cpus}', method, f'{TIMEOUT}'], stdout=subprocess.PIPE)
            tmp = list_files.stdout.decode('utf-8').strip().split()
            res += [[cpus, int(tmp[4]), float(tmp[10])]]
            print(f'{cpus}, {tmp[4]}, {tmp[10]}')
            with open('mpstat') as f:
                tmp = f.read().strip().split('\n')[-1].split()
                assert (tmp[0] == "Average:")
                res_mpstat += [[cpus, float(tmp[2]), float(tmp[4]), float(tmp[5]), float(tmp[6]), float(tmp[7]), float(tmp[11])]]

        results[method] = res
        
        if dev in ['--cpu']:
            plot(res, method)
        plot_mpstat(res_mpstat, method + '_mpstat')

    if dev == '--cache':
        plot_cache(results)


io_.run(MAX_CPUS_COUNT, TIMEOUT, ['--iomix', '--ioprio'])
io_.run(MAX_CPUS_COUNT, TIMEOUT, ['--madvise', '--memrate'])
io_.run(MAX_CPUS_COUNT, TIMEOUT, ['--sockdiag', '--dccp'])
io_.run(MAX_CPUS_COUNT, TIMEOUT, ['--sigpipe'])  # '--oom-pipe' crashes/freezes host!!! Be careful
io_.run(MAX_CPUS_COUNT, TIMEOUT, ['--resched'])

i = 10
res = []
res_mpstat = []
while i <= 10**6:
    list_files = subprocess.run(['./sched-helper', f'{i}', f'{TIMEOUT}'], stdout=subprocess.PIPE)
    tmp = list_files.stdout.decode('utf-8').strip().split()
    res += [[i, int(tmp[4]), float(tmp[10])]]
    with open('mpstat') as f:
        tmp = f.read().strip().split('\n')[-1].split()
        assert (tmp[0] == "Average:")
        res_mpstat += [[i, float(tmp[2]), float(tmp[4]), float(tmp[5]), float(tmp[6]), float(tmp[7]), float(tmp[11])]]

    print(res[-1])
    i *= 10

plot(res, '--sched-runtime')
plot_mpstat(res_mpstat, '--sched-runtime' + '_mpstat')

print("done")
