#ifndef _SFKCONF_HPP_
#define _SFKCONF_HPP_

#ifdef USERCONF
   #include "sfkuser.hpp"
#elif (defined(SFKMINCORE) || defined(SWINST))
   #define sfk_prog 0
#elif (defined(SFKMAXCORE))
   #define sfk_prog      0
   #define sfk_file_net  1
   #define sfk_file_zip  1
   #define sfk_zip       1
   #define sfk_file_walk 1
   #define sfk_file_snap 1
   #define sfk_net       1
   #define sfk_net_tcp   1
   #define sfk_net_udp   1
   #define sfk_net_ftp   1
   #define sfk_text_match 1
#elif (defined(DVOSE))
   #define sfk_prog      0
   #define sfk_file_net  1
   #define sfk_file_zip  0
   #define sfk_zip       0
   #define sfk_file_walk 1
   #define sfk_file_snap 1
   #define sfk_net       1
   #define sfk_net_tcp   1
   #define sfk_net_udp   1
   #define sfk_net_ftp   1
   #define sfk_text_match 1
#elif (defined(DVIEW))
   #define sfk_prog      0
   #define sfk_file_net  1
   #define sfk_file_zip  1
   #define sfk_zip       1
   #define sfk_file_walk 1
   #define sfk_file_snap 1
   #define sfk_net       1
   #define sfk_net_tcp   1
   #define sfk_net_udp   1
   #define sfk_net_ftp   1
   #define sfk_text_match 1
#elif (defined(IVIEW))
   #define sfk_prog      0
   #define sfk_file_net  0
   #define sfk_file_zip  0
   #define sfk_zip       0
   #define sfk_file_walk 1
   #define sfk_file_snap 1
   #define sfk_net       0
   #define sfk_net_tcp   0
   #define sfk_net_udp   0
   #define sfk_net_ftp   0
#elif (defined(USE_SFK_BASE))
   #define sfk_prog     0
#else
   // default: build sfk command line program
   #define sfk_prog     1
   #define sfk_file_net 1
   #define sfk_file_zip 1
   #define sfk_zip      1
#endif

#ifndef sfk_file_walk
 #define sfk_file_walk 0
#endif
#ifndef sfk_file_list
 #define sfk_file_list 0
#endif

#ifndef  sfk_file_net   // Coi's with http:// access
 #define sfk_file_net 0
#endif
#ifndef  sfk_file_zip   // XE only
 #define sfk_file_zip 0
#endif

#ifndef  sfk_net
 #define sfk_net 0
#endif
#ifndef  sfk_zip        // zip/unzip and office read
 #define sfk_zip 0
#endif
#ifndef  sfk_net_udp
 #define sfk_net_udp 0
#endif
#ifndef  sfk_net_tcp
 #define sfk_net_tcp 0
#endif
#ifndef  sfk_net_wget
 #define sfk_net_wget 0
#endif
#ifndef  sfk_net_ftp
 #define sfk_net_ftp 0
#endif
#ifndef  sfk_net_ftpserv
 #define sfk_net_ftpserv 0
#endif
#ifndef  sfk_net_proxy
 #define sfk_net_proxy 0
#endif
#ifndef  sfk_net_knx
 #define sfk_net_knx 0
#endif
#ifndef  sfk_file_find
 #define sfk_file_find 0
#endif
#ifndef  sfk_file_replace
 #define sfk_file_replace 0
#endif
#ifndef  sfk_file_copy
 #define sfk_file_copy 0
#endif
#ifndef  sfk_file_run
 #define sfk_file_run 0
#endif
#ifndef  sfk_file_filter
 #define sfk_file_filter 0
#endif
#ifndef  sfk_file_conv
 #define sfk_file_conv 0
#endif
#ifndef  sfk_file_rename
 #define sfk_file_rename 0
#endif
#ifndef  sfk_file_del
 #define sfk_file_del 0
#endif
#ifndef  sfk_file_dupscan
 #define sfk_file_dupscan 0
#endif
#ifndef  sfk_file_alias
 #define sfk_file_alias 0
#endif
#ifndef  sfk_file_touch
 #define sfk_file_touch 0
#endif
#ifndef  sfk_file_inst
 #define sfk_file_inst 0
#endif
#ifndef  sfk_file_md5
 #define sfk_file_md5 0
#endif
#ifndef  sfk_file_snap
 #define sfk_file_snap 0
#endif
#ifndef  sfk_file_reflist
 #define sfk_file_reflist 0
#endif
#ifndef  sfk_file_deblank
 #define sfk_file_deblank 0
#endif
#ifndef  sfk_file_tab
 #define sfk_file_tab 0
#endif
#ifndef  sfk_file_version
 #define sfk_file_version 0
#endif
#ifndef  sfk_file_checkdisk
 #define sfk_file_checkdisk 0
#endif
#ifndef  sfk_text_sort
 #define sfk_text_sort 0
#endif
#ifndef  sfk_file_index
 #define sfk_file_index 0
#endif
#ifndef  sfk_sfk_prog
 #define sfk_sfk_prog 0
#endif
#ifndef  sfk_text_uni
 #define sfk_text_uni 0
#endif
#ifndef  sfk_file_join
 #define sfk_file_join 0
#endif
#ifndef  sfk_text_data
 #define sfk_text_data 0
#endif
#ifndef  sfk_text_match
 #define sfk_text_match 0
#endif
#ifndef  sfk_file_bintext
 #define sfk_file_bintext 0
#endif
#ifndef  sfk_file_test
 #define sfk_file_test 0
#endif
#ifndef  sfk_file_wav
 #define sfk_file_wav 0
#endif
#ifndef  sfk_help
 #define sfk_help 0
#endif
#ifndef  sfk_file_patch
 #define sfk_file_patch 0
#endif
#ifndef  sfk_sfk_chain
 #define sfk_sfk_chain 0
#endif

#if (sfk_file_net || sfk_file_zip)
 #ifdef SFKNOVFILE
  #error "config conflict: cannot accept file_file_net/zip with SFKNOVFILE"
 #endif
 #define VFILEBASE
 #if (sfk_file_net)
  #define VFILENET
 #endif
#endif

#if (sfk_zip)
 #define SFKPACK
 #define SFKOFFICE
#endif

#endif //  _SFKCONF_HPP_
