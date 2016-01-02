#!/bin/bash

pushd ../..
export LOST_BUILDMK_ROOT="`pwd`"
source config.sh
popd

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
$LOST_TOOLS_MKE2FS  -T ext2 -F -q scratch.img

run_test main 1
run_test main 2
run_test main 3
run_test main 4
run_test main 5

run_test main_thread 1
run_test main_thread 2
run_test main_thread 3
