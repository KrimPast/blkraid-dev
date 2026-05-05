## Fio tests
Block driver has next fio tests
- sequential read-write 4/8/16k
- random read-write 4/8/16k
## Before using this
1. Move to root directory and build the project
    ```sh
    make
    ```
2. Run
    ```sh
    sh ./init_storages_and_blkraid.sh
    ```
    It will init storages and RAID-driver for tests.
## Usage
- Make sure you have installed fio in your distribution.
- ```sh
  fio <test_path>
  ```
