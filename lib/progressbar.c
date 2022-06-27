/**
* \file
* \author Trevor Fountain
* \author Johannes Buchner
* \author Erik Garrison
* \date 2010-2014
* \copyright BSD 3-Clause
*
* progressbar -- a C class (by convention) for displaying progress
* on the command line (to stderr).
*/

#include <termcap.h>  /* tgetent, tgetnum */
#include <assert.h>
#include <limits.h>
#include "progressbar.h"

///  How wide we assume the screen is if termcap fails.
enum { DEFAULT_SCREEN_WIDTH = 80 };
/// The smallest that the bar can ever be (not including borders)
enum { MINIMUM_BAR_WIDTH = 10 };
/// The format in which the estimated remaining time will be reported
static const char *const ETA_FORMAT = "ETA:%2dh%02dm%02ds";
/// The format in which the completion time will be reported
static const char *const ELAPSED_FORMAT = "    %2dh%02dm%02ds";
/// The maximum number of characters that the ETA_FORMAT can ever yield
enum { ETA_FORMAT_LENGTH  = 13 };
/// Amount of screen width taken up by whitespace (i.e. whitespace between label/bar/ETA components)
enum { WHITESPACE_LENGTH = 2 };
/// The amount of width taken up by the border of the bar component.
enum { BAR_BORDER_WIDTH = 2 };

/// Models a duration of time broken into hour/minute/second components. The number of seconds should be less than the
/// number of seconds in one minute, and the number of minutes should be less than the number of minutes in one hour.
typedef struct {
  int hours;
  int minutes;
  int seconds;
} progressbar_time_components;

static void progressbar_draw(progressbar *bar);
static void progressbar_assign_values(progressbar *bar, const char *format,
                                      const char *tumbler_format);

/**
* Create a new progress bar with the specified label, max number of steps, and format string.
* Note that `format` must be exactly four characters long, e.g. "<- >" to render a progress
* bar like "<------    >". Returns NULL if there isn't enough memory to allocate a progressbar
*/
progressbar *progressbar_new_with_format(const char *label, long max, const char *format)
{
  progressbar *pb = malloc(sizeof(progressbar));
  if(pb == NULL) {
    return NULL;
  }

  pb->max = max;
  pb->value = 0;
  progressbar_assign_values(pb, format, NULL);

  progressbar_update_label(pb, label);
  progressbar_draw(pb);

  return pb;
}

progressbar *progressbar_new_percent_with_format(const char *label, const char *format)
{
  progressbar *pb = malloc(sizeof(progressbar));
  if(pb == NULL) {
    return NULL;
  }

  pb->max = -1;
  pb->percent = 0.0;
  progressbar_assign_values(pb, format, NULL);

  progressbar_update_label(pb, label);
  progressbar_draw(pb);
}

/**
* Create a new progress bar with the specified label, max number of steps, format string,
* and tumbler format string.
* Note that 'format` must be exactly four characters long, e.g. "<- >" to render a progress
* bar like "<------    >".   'tumbler_format' can be any length.
* Returns NULL if there isn't enough memory to allocate a progressbar
*/
progressbar *progressbar_new_with_format_and_tumbler(const char *label, long max,
                                                     const char *format,
                                                     const char *tumbler_format)
{
  progressbar *pb = malloc(sizeof(progressbar));
  if(pb == NULL) {
    return NULL;
  }

  pb->max = max;
  pb->value = 0;
  progressbar_assign_values(pb, format, tumbler_format);

  progressbar_update_label(pb, label);
  progressbar_draw(pb);

  return pb;
}

progressbar *progressbar_new_percent_with_format_and_tumbler(const char *label,
                                                             const char *format,
                                                             const char *tumbler_format)
{
  progressbar *pb = malloc(sizeof(progressbar));
  if(pb == NULL) {
    return NULL;
  }

  pb->max = -1;
  pb->percent = 0.0;
  progressbar_assign_values(pb, format, tumbler_format);

  progressbar_update_label(pb, label);
  progressbar_draw(pb);

  return pb;
}

/**
* Create a new progress bar with the specified label and max number of steps.
*/
progressbar *progressbar_new(const char *label, long max)
{
  return progressbar_new_with_format(label, max, "|= |");
}

progressbar *progressbar_new_percent(const char *label)
{
  return progressbar_new_percent_with_format(label, "|= |");
}

void progressbar_update_label(progressbar *bar, const char *label)
{
  bar->label = label;
}

/**
* Delete an existing progress bar.
*/
void progressbar_free(progressbar *bar)
{
  free(bar);
  bar = NULL;
}

/**
* Set an existing progressbar to `value` steps.
*/
void progressbar_update(progressbar *bar, long value)
{
  bar->value = value;
  progressbar_draw(bar);
}

void progressbar_update_percent(progressbar *bar, double percent)
{
  bar->percent = percent;
  progressbar_draw(bar);
}

/**
* Increment an existing progressbar by a single step.
*/
void progressbar_inc(progressbar *bar)
{
  progressbar_update(bar, bar->value+1);
}

static void progressbar_write_char(FILE *file, const int ch, const size_t times) {
  size_t i;
  for (i = 0; i < times; ++i) {
    fputc(ch, file);
  }
}

static int progressbar_max(int x, int y) {
  return x > y ? x : y;
}

static unsigned int get_screen_width(void) {
  char termbuf[2048];
  if (tgetent(termbuf, getenv("TERM")) >= 0) {
    return tgetnum("co") /* -2 */;
  } else {
    return DEFAULT_SCREEN_WIDTH;
  }
}

static int progressbar_bar_width(int screen_width, int label_length) {
  return progressbar_max(MINIMUM_BAR_WIDTH, screen_width - label_length - ETA_FORMAT_LENGTH - WHITESPACE_LENGTH);
}

static int progressbar_label_width(int screen_width, int label_length, int bar_width) {
  int eta_width = ETA_FORMAT_LENGTH;

  // If the progressbar is too wide to fit on the screen, we must sacrifice the label.
  if (label_length + 1 + bar_width + 1 + ETA_FORMAT_LENGTH > screen_width) {
    return progressbar_max(0, screen_width - bar_width - eta_width - WHITESPACE_LENGTH);
  } else {
    return label_length;
  }
}

static int progressbar_remaining_seconds(const progressbar* bar) {
  double offset = difftime(time(NULL), bar->start);
  if (bar->value > 0 && offset > 0) {
    if (bar->max < 0)
      return (offset / bar->percent) * (1.0 - bar->percent);
    else
      return (offset / (double) bar->value) * (bar->max - bar->value);
  } else {
    return 0;
  }
}

static progressbar_time_components progressbar_calc_time_components(int seconds) {
  int hours = seconds / 3600;
  seconds -= hours * 3600;
  int minutes = seconds / 60;
  seconds -= minutes * 60;

  progressbar_time_components components = {hours, minutes, seconds};
  return components;
}

static void progressbar_draw(progressbar *bar)
{
  int screen_width = get_screen_width();
  int label_length = strlen(bar->label);
  int bar_width = progressbar_bar_width(screen_width, label_length);
  int label_width = progressbar_label_width(screen_width, label_length, bar_width);

  int progressbar_completed = bar->max < 0 ? (bar->percent >= 1.0) : (bar->value >= bar->max);
  int bar_piece_count = bar_width - BAR_BORDER_WIDTH;
  int bar_piece_current = (progressbar_completed)
                          ? bar_piece_count
                          : bar_piece_count
                            * (bar->max < 0
                               ? bar->percent
                               : ((double) bar->value / bar->max));
  bar_piece_current = (progressbar_completed || bar->tumbler_length == 0)
                      ? bar_piece_current
                      : bar_piece_current == 0
                        ? bar_piece_current
                        : bar_piece_current - 1;

  progressbar_time_components eta = (progressbar_completed)
		                            ? progressbar_calc_time_components(difftime(time(NULL), bar->start))
		                            : progressbar_calc_time_components(progressbar_remaining_seconds(bar));

  if (label_width == 0) {
    // The label would usually have a trailing space, but in the case that we don't print
    // a label, the bar can use that space instead.
    bar_width += 1;
  } else {
    // Draw the label
    fwrite(bar->label, 1, label_width, stderr);
    fputc(' ', stderr);
  }

  // Draw the progressbar
  fputc(bar->format.begin, stderr);
  progressbar_write_char(stderr, bar->format.fill, bar_piece_current);
  if(bar->tumbler_length > 0 && bar_piece_current < bar_piece_count)
  {
    fputc(bar->tumbler_format[bar->tumbler_pos], stderr);
    bar->tumbler_pos += 1;
    bar->tumbler_pos = bar->tumbler_pos % bar->tumbler_length;
  }
  progressbar_write_char(stderr, bar->format.unfilled, bar_piece_count - bar_piece_current - (bar->tumbler_length == 0 ? 0 : bar_piece_current == bar_piece_count ? 0 : 1));
  fputc(bar->format.end, stderr);

  // Draw the ETA
  fputc(' ', stderr);
  fprintf(stderr, progressbar_completed ? ELAPSED_FORMAT : ETA_FORMAT, eta.hours, eta.minutes, eta.seconds);
  fputc('\r', stderr);
}

/**
* Finish a progressbar, indicating 100% completion, and free it.
*/
void progressbar_finish(progressbar *bar)
{
  // Make sure we fill the progressbar so things look complete.
  if(bar->max < 0)
    bar->percent = 1.0;
  progressbar_draw(bar);

  // Print a newline, so that future outputs to stderr look prettier
  fprintf(stderr, "\n");

  // We've finished with this progressbar, so go ahead and free it.
  progressbar_free(bar);
}

static void progressbar_assign_values(progressbar *bar, const char *format,
                                      const char *tumbler_format)
{
  bar->start = time(NULL);
  assert(4 == strlen(format) && "format must be four characters in length");
  bar->format.begin = format[0];
  bar->format.fill = format[1];
  bar->format.unfilled = format[2];
  bar->format.end = format[3];
  bar->tumbler_format = tumbler_format;
  bar->tumbler_length = tumbler_format ? strlen(tumbler_format) : 0;
  bar->tumbler_pos = 0;
}
