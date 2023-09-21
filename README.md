# OS
## Installing XV6 at Apple silicon devices (M1/M2)
1. Install [UTM](https://mac.getutm.app/)
2. Install [Ubuntu Server Arm64 into UTM](https://docs.getutm.app/guides/ubuntu/) (you can use another linux dist)
3. Boot into UTM:
- `sudo apt update & sudo apt install gcc gcc-riscv-linux-gnu qemu-system-riscv64 git`
- `git clone https://github.com/mit-pdos/xv6-riscv.git`
- `cd xv6-riscv`
- `make TOLLPREFIX=riscv-linux-gnu-`
- `make TOLLPREFIX=riscv-linux-gnu- qemu`

**DONE**
