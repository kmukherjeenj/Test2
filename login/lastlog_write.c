/* Write a lastlog entry.
   Copyright (C) 2020 Free Software Foundation, Inc.
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

#include <utmp.h>
#include <stdbool.h>
#include <not-cancel.h>
#include <lastlog-compat.h>

ssize_t
lastlog_write (const char *file, uid_t uid, const struct lastlog *ll)
{
  int fd = __open_nocancel (file, O_WRONLY | O_LARGEFILE | O_CLOEXEC);
  if (fd == -1)
    return -1;

  size_t llsize;
  struct lastlog_compat llcompat;
  const void *data;

  if (is_path_lastlog_compat (file))
    {
      llcompat.ll_time = ll->ll_time;
      memcpy (llcompat.ll_line, ll->ll_line, UT_LINESIZE);
      memcpy (llcompat.ll_host, ll->ll_host, UT_HOSTSIZE);
      llsize = sizeof (struct lastlog_compat);
      data = &llcompat;
    }
  else
    {
      llsize = sizeof (struct lastlog);
      data = ll;
    }

  off64_t off = llsize * uid;
  ssize_t r = __pwrite64_nocancel (fd, data, llsize, off);
  __close_nocancel_nostatus (fd);

  /* We need to return the expected 'struct lastlog' size in case of
     success.  */
  if (r == llsize && data == &llcompat)
    r = sizeof (struct lastlog);

  return r;
}