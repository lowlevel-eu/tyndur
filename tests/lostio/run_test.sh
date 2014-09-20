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

    $QEMU -nographic -kernel $kernel -initrd "$modules","$testcase $subtest" -hda scratch.img | filter_output
#    $QEMU -kernel $kernel -initrd "$modules","$testcase $subtest" -hda scratch.img -serial stdio | filter_output
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
