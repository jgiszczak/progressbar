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

#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Progressbar data structure (do not modify or create directly)
 */
typedef struct _progressbar_t
{
  /// maximum value
  long max;
  /// current value
  union {
    long value;
    double percent;
  };

  /// time progressbar was started
  time_t start;

  /// label
  const char *label;

  /// characters for the beginning, filling and end of the
  /// progressbar. E.g. |###    | has |# |
  struct {
    unsigned char begin;
    unsigned char fill;
    unsigned char unfilled;
    unsigned char end;
  } format;

  /// characters for the optional tumbler.
  /// E.g. "/-\\|"
  const char *tumbler_format;
  size_t tumbler_length;
  unsigned int tumbler_pos;
} progressbar;

/// Create a new progressbar with the specified label.
///
/// The progress bar must be updated with progress_update_percent().
///
/// @param label The label that will prefix the progressbar.
///
/// @return A progressbar configured with the provided arguments. Note that the user is responsible for disposing
///         of the progressbar via progressbar_finish when finished with the object.
progressbar *progressbar_new_percent(const char *label);

/// Create a new progressbar with the specified label and number of steps.
///
/// @param label The label that will prefix the progressbar.
/// @param max The number of times the progressbar must be incremented before it is considered complete,
///            or, in other words, the number of tasks that this progressbar is tracking.
///
/// @return A progressbar configured with the provided arguments. Note that the user is responsible for disposing
///         of the progressbar via progressbar_finish when finished with the object.
progressbar *progressbar_new(const char *label, long max);

/// Create a new progressbar with the specified label, number of steps, and format string.
///
/// @param label The label that will prefix the progressbar.
/// @param max The number of times the progressbar must be incremented before it is considered complete,
///            or, in other words, the number of tasks that this progressbar is tracking.
/// @param format The format of the progressbar. The string provided must be four characters, and it will
///               be interpreted with the first character as the left border of the bar, the second
///               character the filled bar, the third character the unfilled bar, and the fourth character
///               as the right border of the bar. For example,
///               "<- >" will result in a bar formatted as "<------     >".
///
/// @return A progressbar configured with the provided arguments. Note that the user is responsible for disposing
///         of the progressbar via progressbar_finish when finished with the object.
progressbar *progressbar_new_with_format(const char *label, long max, const char *format);
progressbar *progressbar_new_percent_with_format(const char *label, const char *format);


/// Create a new progressbar with the specified label, number of steps, and format string.
///
/// @param label The label that will prefix the progressbar.
/// @param max The number of times the progressbar must be incremented before it is considered complete,
///            or, in other words, the number of tasks that this progressbar is tracking.
/// @param format The format of the progressbar. The string provided must be four characters, and it will
///               be interpreted with the first character as the left border of the bar, the second
///               character the filled bar, the third charact the unfilled bar, and the fourth character
///               as the right border of the bar. For example,
///               "<- >" would result in a bar formatted like "<------     >".
/// @param tumbler_format The format of the optional tumbler, suggested for use with very long-running tasks.
///                       The string provided may be any number of characters which will be displayed in
///                       sequence from left to right, repeatedly.  For example,
///                       "/-\\|" will result in a bar formatted as "<-----/     >", "<------     >", "<-----\     >", "<-----|     >"
///
/// @return A progressbar configured with the provided arguments. Note that the user is responsible for disposing
///         of the progressbar via progressbar_finish when finished with the object.
progressbar *progressbar_new_with_format_and_tumbler(const char *label, long max, const char *format, const char *tumbler_format);
progressbar *progressbar_new_percent_with_format_and_tumbler(const char *label, const char *format, const char *tumbler_format);

/// Free an existing progress bar. Don't call this directly; call *progressbar_finish* instead.
void progressbar_free(progressbar *bar);

/// Increment the given progressbar. Don't increment past the initialized # of steps, though.
void progressbar_inc(progressbar *bar);

/// Set the current status on the given progressbar.
void progressbar_update(progressbar *bar, long value);

/// Set the current status on the given percentage mode progressbar.
void progressbar_update_percent(progressbar *bar, double percent);

/// Set the label of the progressbar. Note that no rendering is done. The label is simply set so that the next
/// rendering will use the new label. To immediately see the new label, call progressbar_draw.
/// Does not update display or copy the label
void progressbar_update_label(progressbar *bar, const char *label);

/// Finalize (and free!) a progressbar. Call this when you're done, or if you break out
/// partway through.
void progressbar_finish(progressbar *bar);

#ifdef __cplusplus
}
#endif

#endif
