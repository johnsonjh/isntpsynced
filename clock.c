/* SPDX-License-Identifier: MIT-0 */
/* Copyright (c) 2025 Jeffrey H. Johnson <trnsz@pobox.com> */

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__sun) || \
    defined(__illumos__) || defined(__linux__) && !defined(__ANDROID__)
# include <sys/timex.h>
#endif

static int
is_ntp_sync (void) // -1 == Failed to check     0 == Not synchronized
{                  //  1 == Maybe synchronized  2 == Synchronized OK
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__sun) || \
    defined(__illumos__) || defined(__linux__) && !defined(__ANDROID__)
  struct timex tx = { 0 };
# if defined(__FreeBSD__) || defined(__NetBSD__) || \
     defined(__sun) || defined(__illumos__)
  int result = ntp_adjtime (&tx);
# else
  int result = adjtimex (&tx);
# endif
  if (-1 == result)
    return -1;
# if defined(__sun) || defined(__illumos__)
  if (tx.status & STA_UNSYNC)
# else
  if (TIME_OK != result)
# endif
    if (1000000 >= tx.maxerror)
      return 1;
    else
      return 0;
  else
    return 2;
#else
  return -1;
#endif
}

int
main (void)
{
  int result = is_ntp_sync();

  if (0 == result)
    (void)fprintf(stdout, "Clock is NOT synchronized.\r\n");
  else if (1 == result)
    (void)fprintf(stdout, "Clock is probably synchronized.\r\n");
  else if (2 == result)
    (void)fprintf(stdout, "Clock is synchronized.\r\n");
  else
    (void)fprintf(stdout, "Unable to check clock synchronization.\r\n");
}
