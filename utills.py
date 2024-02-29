from matplotlib import pyplot as plt

def plot(table: list, title: str):
    cpus = [i[0] for i in table]
    bops = [i[1] for i in table]
    load = [i[2] for i in table]

    _, axs = plt.subplots(1, 2, figsize=(12, 6))

    axs[0].autoscale(enable=True, axis='y', tight=False)
    axs[0].set_ylabel("bogo ops")
    axs[0].plot(cpus, bops)
    axs[0].set_title(title)

    axs[1].autoscale(enable=True, axis='y', tight=False)
    axs[1].set_ylabel("CPU usage")
    axs[1].plot(cpus, load)
    axs[1].set_title(title)

    plt.tight_layout()
    plt.savefig(f'res/{title}.png')

def plot_iostat(table: list, title: str):
    cpus = [i[0] for i in table]
    bops = [i[1] for i in table]
    load = [i[2] for i in table]

    _, axs = plt.subplots(1, 2, figsize=(12, 6))

    axs[0].autoscale(enable=True, axis='y', tight=False)
    axs[0].set_ylabel("read")
    axs[0].plot(cpus, bops)
    axs[0].set_title(title)

    axs[1].autoscale(enable=True, axis='y', tight=False)
    axs[1].set_ylabel("write")
    axs[1].plot(cpus, load)
    axs[1].set_title(title)

    plt.tight_layout()
    plt.savefig(f'res/{title}.png')

def plot_mpstat(table: list, title: str):
    print(table)
    cpus = [i[0] for i in table]
    data = [[i[ind] for i in table] for ind in range(1, len(table[0]))]
    print(data)
    labels = ['usr', 'sys', 'iowait', 'irq', 'soft', 'idle']


    _, axs = plt.subplots(2, 3, figsize=(12, 6))
    for i in range(2):
        for j in range(3):
            axs[i][j].autoscale(enable=True, axis='y', tight=False)
            axs[i][j].plot(cpus, data[i * 3 + j])
            axs[i][j].set_title(labels[i * 3 + j])

    plt.tight_layout()
    plt.savefig(f'res/{title}.png')

def plot_cache(results: dict):
    cpus_1 = [i[0] for i in results['']]
    bops_1 = [i[1] for i in results['']]
    load_1 = [i[2] for i in results['']]

    cpus_2 = [i[0] for i in results['--cache-fence']]
    bops_2 = [i[1] for i in results['--cache-fence']]
    load_2 = [i[2] for i in results['--cache-fence']]

    fig, axs = plt.subplots(1, 2, figsize=(12, 6))

    axs[0].plot(cpus_1, bops_1, label='off', marker='o')
    axs[0].plot(cpus_2, bops_2, label='on', marker='x')
    axs[0].set_xlabel("cpu count")
    axs[0].set_ylabel("bogo ops")
    axs[0].set_title("Fences on/off")
    axs[0].legend()
    axs[0].autoscale(enable=True, axis='y', tight=False)

    axs[1].plot(cpus_1, load_1, label='Fences off', marker='o')
    axs[1].plot(cpus_2, load_2, label='Fences on', marker='x')
    axs[1].set_xlabel("cpu count")
    axs[1].set_ylabel("CPU usage")
    axs[1].set_title("Fences on/off")
    axs[1].legend()
    axs[1].autoscale(enable=True, axis='y', tight=False)

    plt.tight_layout()
    plt.savefig(f'res/fences.png')


    cpus_1 = [i[0] for i in results['--cache-level 1']]
    bops_1 = [i[1] for i in results['--cache-level 1']]
    load_1 = [i[2] for i in results['--cache-level 1']]

    cpus_2 = [i[0] for i in results['--cache-level 2']]
    bops_2 = [i[1] for i in results['--cache-level 2']]
    load_2 = [i[2] for i in results['--cache-level 2']]

    cpus_3 = [i[0] for i in results['--cache-level 3']]
    bops_3 = [i[1] for i in results['--cache-level 3']]
    load_3 = [i[2] for i in results['--cache-level 3']]


    fig, axs = plt.subplots(1, 2, figsize=(12, 6))

    axs[0].plot(cpus_1, bops_1, label='1', marker='o')
    axs[0].plot(cpus_2, bops_2, label='2', marker='x')
    axs[0].plot(cpus_3, bops_3, label='3', marker='*')
    axs[0].set_xlabel("cpu count")
    axs[0].set_ylabel("bogo ops")
    axs[0].set_title("cache level")
    axs[0].legend()
    axs[0].autoscale(enable=True, axis='y', tight=False)

    axs[1].plot(cpus_1, load_1, label='1', marker='o')
    axs[1].plot(cpus_2, load_2, label='2', marker='x')
    axs[1].plot(cpus_3, load_3, label='3', marker='*')
    axs[1].set_xlabel("cpu count")
    axs[1].set_ylabel("bogo ops")
    axs[1].set_title("cache level")
    axs[1].legend()
    axs[1].autoscale(enable=True, axis='y', tight=False)

    plt.tight_layout()
    plt.savefig(f'res/cache-levels.png')