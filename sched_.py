import subprocess

from matplotlib import pyplot as plt
from utills import plot, plot_iostat, plot_mpstat
from statistics import mean 

def run(MAX_CPUS_COUNT: int, TIMEOUT: int, methods: list):
    for method in methods:
        res = []
        res_iostat = []
        res_mpstat = []
        for cpus in range(1, MAX_CPUS_COUNT+1):
            list_files = subprocess.run(['./stress-ng-helper', method, f'{cpus}', '', f'{TIMEOUT}'], stdout=subprocess.PIPE)
            tmp = list_files.stdout.decode('utf-8').strip().split()
            res += [[cpus, int(tmp[4]), float(tmp[10])]]
            with open('iostat') as f:
                tmp = f.read().strip().split('\n')

                assert (len(tmp) == TIMEOUT)
                res_iostat += [[cpus, mean([float(i.split()[2]) for i in tmp]), mean([float(i.split()[3]) for i in tmp])]]
            with open('mpstat') as f:
                tmp = f.read().strip().split('\n')[-1].split()
                assert (tmp[0] == "Average:")
                res_mpstat += [[cpus, float(tmp[2]), float(tmp[4]), float(tmp[5]), float(tmp[6]), float(tmp[7]), float(tmp[11])]]
        plot(res, method)
        plot_iostat(res_iostat, method + '_iostat')
        plot_mpstat(res_mpstat, method + '_mpstat')


if __name__ == '__main__':
    run(2, 5, ['--oom-pipe', '--sigpipe'])
