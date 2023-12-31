# OS

Labs are done on [`f5b93ef12f7159f74f80f94729ee4faabe42c360`](https://github.com/mit-pdos/xv6-riscv/tree/f5b93ef12f7159f74f80f94729ee4faabe42c360) commit of XV6

## Installing XV6 on Apple Silicon devices (M1/M2)
1. Install [Homebrew](http://brew.sh/) *(if not installed yet)*:
   - `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`
2. Get [riscv-tools](https://github.com/riscv-software-src/homebrew-riscv) *(it will build it from source, so be patient)*:
   - `brew tap riscv-software-src/riscv`
   - `brew install riscv-tools`
3. Get [Qemu](https://www.qemu.org/):
   - `brew install qemu`
4. Clone xv6 repo:
   - `git clone https://github.com/mit-pdos/xv6-riscv.git`
5. Build xv6:
    - `cd xv6-riscv`
    - `make`
6. Run xv6:
    - `make qemu`

**DONE**

## Installing XV6 on Apple Silicon devices (M1/M2) via UTM
1. Install [UTM](https://mac.getutm.app/)
2. Install [Ubuntu Server Arm64 into UTM](https://docs.getutm.app/guides/ubuntu/) *(you can use another linux dist)*
3. Boot into UTM:
   - `sudo apt update & sudo apt install gcc gcc-riscv-linux-gnu qemu-system-riscv64 git`
   - `git clone https://github.com/mit-pdos/xv6-riscv.git`
   - `cd xv6-riscv`
   - `make TOOLPREFIX=riscv-linux-gnu-`
   - `make TOOLPREFIX=riscv-linux-gnu- qemu`

**DONE**

## Additional tasks
1. Task 1 for Lab1: \
   Write buffered chanel from Go with usage example
