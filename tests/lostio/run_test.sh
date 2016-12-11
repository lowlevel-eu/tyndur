#!/bin/bash

export PATH=$PATH:/usr/local/sbin:/usr/sbin:/sbin

kernel=../../build/output/kernel/tyndur2
modules="../../build/output/modules/init,../../build/output/modules/pci,../../build/output/modules/ata,../../build/output/modules/ext2,../../build/output/modules/servmgr root out ata pci ext2"

function filter_output()
{
    grep '^*' |\
         sed -e "s/PASS/\x1b[32mPASS\x1b[0m/" |\
         sed -e "s/FAIL/\x1b[31mFAIL\x1b[0m/" |\
         sed -e "s/ERROR/\x1b[33mERROR\x1b[0m/"
}

function run_test()
{
    local testcase=$1
    local subtest=$2

    $QEMU -nographic -kernel $kernel -initrd "$modules","$testcase $subtest" -drive file=scratch.img,format=raw -device isa-debug-exit | filter_output
#    $QEMU -kernel $kernel -initrd "$modules","$testcase $subtest"  -drive file=scratch.img,format=raw -device isa-debug-exit -serial stdio | filter_output
}

dd if=/dev/zero of=scratch.img bs=1M count=32

run_test 001 1
run_test 001 2
run_test 001 3
run_test 001 4
run_test 001 5
run_test 001 6
run_test 001 7
run_test 001 8
run_test 001 9
run_test 001 10
run_test 001 11

dd if=/dev/zero of=scratch.img bs=1M count=32
mke2fs -Fq scratch.img

run_test 002 1
run_test 002 2

dd if=/dev/zero of=scratch.img bs=1M count=32
mke2fs -Fq scratch.img
dd if=/dev/zero of=inner.ext2 bs=1M count=28
mke2fs -Fq inner.ext2
echo "Ein Testtext zum Texten des Tests und zum Testen des Texts." > test.txt
debugfs -wR "write test.txt test.txt" inner.ext2
debugfs -wR "write inner.ext2 inner.ext2" scratch.img

run_test 003 1
run_test 003 2
run_test 003 3
