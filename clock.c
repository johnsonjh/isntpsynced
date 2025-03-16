#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__linux__)
# include <dirent.h>
# include <sys/timex.h>
# include <unistd.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
# include <sys/timex.h>
#elif defined(__sun) || defined(__illumos__)
# include <sys/time.h>
# include <sys/timex.h>
#endif

#if defined(__linux__)
static bool
is_process_running (const char *process_name)
{
  DIR *proc_dir = opendir ("/proc");

  if (!proc_dir)
    return false;

  struct dirent *entry;
  while (NULL != (entry = readdir (proc_dir))) {
    if (DT_DIR == entry->d_type) {
      char cmdline_path[256];
      (void)snprintf (cmdline_path, sizeof (cmdline_path), "/proc/%s/cmdline", entry->d_name);
      FILE *cmdline_file = fopen (cmdline_path, "r");
      if (cmdline_file) {
        char cmdline[256];
        if (NULL != fgets (cmdline, sizeof (cmdline), cmdline_file))
          if (NULL != strstr (cmdline, process_name)) {
            (void)fclose (cmdline_file);
            (void)closedir (proc_dir);
            return true;
          }
        (void)fclose (cmdline_file);
      }
    }
  }

  (void)closedir (proc_dir);
  return false;
}
#endif

static bool
is_ntp_synchronized (void)
{
#if defined(__linux__)
  struct timex tx = { 0 };
  int result = adjtimex (&tx);

  if (-1 == result)
    return false;

  return result == TIME_OK;

#elif defined(__FreeBSD__) || defined(__NetBSD__)
  struct timex tx = { 0 };
  int result = ntp_adjtime (&tx);
  if (-1 == result)
    return false;

  return result == TIME_OK;

#elif defined(__sun) || defined(__illumos__)
  struct timex tx = { 0 };
  int result = ntp_adjtime (&tx);
  if (-1 == result)
    return false;

  return !(tx.status & STA_UNSYNC);

#else /* if defined( __linux__ ) */
  (void)fprintf (stderr, "Unsupported operating system.\n");
  return false;

#endif /* if defined( __linux__ ) */
}

int
main (void)
{
#if defined(__linux__)
  struct timex tx = { 0 };
  int result = adjtimex (&tx);
  if (-1 == result) {
    perror ("adjtimex failed");
    return EXIT_FAILURE;
  }

  (void)fprintf (stdout, "Max error: %ld seconds\n", tx.maxerror / 1000000);
  (void)fprintf (stdout, "Estimated error: %ld seconds\n", tx.esterror / 1000000);

  if (TIME_OK != result)
    if (tx.maxerror / 1000000 < 1 && tx.esterror / 1000000 < 1)
      (void)fprintf (stdout, "The time is probably synchronized despite UNSYNC flag.\n");

#elif defined(__FreeBSD__) || defined(__NetBSD__)
  struct timex tx = { 0 };
  int result = ntp_adjtime (&tx);

  if (-1 == result) {
    perror ("ntp_adjtime failed");
    return EXIT_FAILURE;
  }

  (void)fprintf (stdout, "Max error: %ld seconds\n", tx.maxerror / 1000000);
  (void)fprintf (stdout, "Estimated error: %ld seconds\n", tx.esterror / 1000000);

  if (TIME_OK != result)
    if (tx.maxerror / 1000000 < 1 || tx.esterror / 1000000 < 1)
      (void)fprintf (stdout, "The time is probably synchronized despite the lack of TIME_OK status.\n");
    else
      (void)fprintf (stderr, "The system clock is NOT synchronized with an NTP server.\n");

#elif defined(__sun) || defined(__illumos__)
  struct timex tx = { 0 };
  int result = ntp_adjtime (&tx);

  if (-1 == result) {
    perror ("ntp_adjtime failed");
    return EXIT_FAILURE;
  }

  (void)fprintf (stdout, "Max error: %d seconds\n", tx.maxerror / 1000000);
  (void)fprintf (stdout, "Estimated error: %d seconds\n", tx.esterror / 1000000);

  if (tx.status & STA_UNSYNC)
    if (tx.maxerror / 1000000 < 1 || tx.esterror / 1000000 < 1)
      (void)fprintf (stdout, "The time is probably synchronized despite UNSYNC flag.\n");
    else
      (void)fprintf (stderr, "The system clock is NOT synchronized with an NTP server.\n");
#endif

  if (is_ntp_synchronized ())
    (void)fprintf (stdout, "The system clock is synchronized with an NTP server.\n");
  else {
    (void)fprintf (stderr, "The system clock is NOT synchronized with an NTP server.\n");
#if defined(__linux__)
    if (is_process_running ("chronyd"))
      (void)fprintf (stdout, "Chrony is running.\n");
    else if (is_process_running ("systemd-timesyncd"))
      (void)fprintf (stdout, "systemd-timesyncd is running.\n");
    else if (is_process_running ("ntpd") || is_process_running ("ntpd4") || is_process_running ("xntpd"))
      (void)fprintf (stdout, "NTPD is running.\n");
    else
      (void)fprintf (stderr, "No recognized NTP synchronization service is running.\n");
#endif
  }

  return 0;
}
