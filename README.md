# sg200x-evb boards.

step1:
```
git clone -b master git@github.com:flyingcys/sophpi-sg200x-evb.git
cd sophpi-sg200x-evb
git clone git@github.com:sophgo/host-tools.git
```
step2:
```
source build/cvisetup.sh
defconfig sg2002_wevb_riscv64_sd
clean_all
build_all
```
