/* Socket timestamp conversion routines.
   Copyright (C) 2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <kernel-features.h>

#ifndef __ASSUME_TIME64_SYSCALLS
# include <stdint.h>
# include <string.h>
# include <sys/socket.h>
/* The kernel header with SO_* constants is used as default for _GNU_SOURCE,
   however the new constants that describe 64-bit time support were added
   only on v5.1.  */
# if !defined(SO_TIMESTAMP_OLD) || !defined(SO_TIMESTAMP_NEW) \
    || !defined(SO_TIMESTAMPNS_OLD) || !defined(SO_TIMESTAMPNS_NEW)
#  include <bits/socket-constants.h>
# endif

/* It converts the first SO_TIMESTAMP or SO_TIMESTAMPNS with 32-bit time and
   appends it to the control buffer.  The 32-bit time field is kept as-is.

   Calls with __TIMESIZE=32 will see the converted 64-bit time control
   messages as spurious control message of unknown type.

   Calls with __TIMESIZE=64 running on pre-time64 kernels will see the
   original message as a spurious control ones of unknown typ while running
   on kernel with native 64-bit time support will only see the time64 version
   of the control message.  */
void
__convert_scm_timestamps (struct msghdr *msg, socklen_t msgsize)
{
  if (msg->msg_control == NULL || msg->msg_controllen == 0)
    return;

  /* The returnted control message format for SO_TIMESTAMP_NEW is a
     'struct __kernel_sock_timeval' while for SO_TIMESTAMPNS_NEW is a
     'struct __kernel_timespec'.  In both case it is essentially two
     uint64_t members.  */
  uint64_t tvts[2];

  struct cmsghdr *cmsg, *last = NULL;
  int type = 0;

  for (cmsg = CMSG_FIRSTHDR (msg);
       cmsg != NULL;
       cmsg = CMSG_NXTHDR (msg, cmsg))
    {
      if (cmsg->cmsg_level != SOL_SOCKET)
	continue;

      switch (cmsg->cmsg_type)
	{
	case SO_TIMESTAMP_OLD:
	  if (type != 0)
	    break;
	  type = SO_TIMESTAMP_NEW;
	  goto common;

	case SO_TIMESTAMPNS_OLD:
	  type = SO_TIMESTAMPNS_NEW;

	/* fallthrough  */
	common:
	  memcpy (tvts, CMSG_DATA (cmsg), sizeof (tvts));
	  break;
	}

      last = cmsg;
    }

  if (last == NULL || type == 0)
    return;

  if (CMSG_SPACE (sizeof tvts) > msgsize - msg->msg_controllen)
    {
      msg->msg_flags |= MSG_CTRUNC;
      return;
    }

  msg->msg_controllen += CMSG_SPACE (sizeof tvts);
  cmsg = CMSG_NXTHDR(msg, last);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = type;
  cmsg->cmsg_len = CMSG_LEN (sizeof tvts);
  memcpy (CMSG_DATA (cmsg), tvts, sizeof tvts);
}
libc_hidden_def (__convert_scm_timestamps)
#endif