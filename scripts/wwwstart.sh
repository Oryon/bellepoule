#!/bin/bash
lighty-enable-mod fastcgi;
#if test ${?} -ne 0 ; then
  #exit -1;
#fi

lighty-enable-mod fastcgi-php;
#if test ${?} -ne 0 ; then
  #exit -1;
#fi

lighty-enable-mod bellepouledebug;
#if test ${?} -ne 0 ; then
  #exit -1;
#fi

/etc/init.d/lighttpd restart;
if test ${?} -ne 0 ; then
  exit -1;
fi

exit 0;
