#!/bin/bash

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

    $QEMU -nographic -kernel $kernel -initrd "$modules","$testcase $subtest" | filter_output
#    $QEMU -kernel $kernel -initrd "$modules","$testcase $subtest" -serial stdio | filter_output
}

run_test main 1
run_test main 2
