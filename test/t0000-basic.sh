#!/bin/sh

test_description='Basics: see if tools are built, etc.'

. ./test-lib.sh

lvm >/dev/null 2>&1
if test $? != 3; then
    echo >&2 'You do not seem to have built lvm yet.'
    exit 1
fi

v=$abs_top_srcdir/tools/version.h
test_expect_success \
  "get version string from $v" \
  'sed -n "/#define LVM_VERSION ./s///p" '"$v"'|sed "s/ .*//" > expected'

test_expect_success \
  'get version of a just-built binary, ensuring PATH is set properly' \
  'lvm pvmove --version|sed -n "1s/.*: *\([0-9][^ ]*\) .*/\1/p" > actual'

test_expect_success \
  'ensure they are the same' \
  'diff -u actual expected'

test_done
