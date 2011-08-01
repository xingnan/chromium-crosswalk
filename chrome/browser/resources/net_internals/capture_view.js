// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This view displays controls for capturing network events.
 */

var CaptureView = (function() {
  // IDs for special HTML elements in capture_view.html
  const MAIN_BOX_ID = 'capture-view-tab-content';
  const BYTE_LOGGING_CHECKBOX_ID = 'capture-view-byte-logging-checkbox';
  const PASSIVELY_CAPTURED_COUNT_ID = 'capture-view-passively-captured-count';
  const ACTIVELY_CAPTURED_COUNT_ID = 'capture-view-actively-captured-count';
  const DELETE_ALL_ID = 'capture-view-delete-all';
  const TIP_ANCHOR_ID = 'capture-view-tip-anchor';
  const TIP_DIV_ID = 'capture-view-tip-div';

  // We inherit from DivView.
  var superClass = DivView;

  /**
   * @constructor
   */
  function CaptureView() {
    // Call superclass's constructor.
    superClass.call(this, MAIN_BOX_ID);

    var byteLoggingCheckbox = $(BYTE_LOGGING_CHECKBOX_ID);
    byteLoggingCheckbox.onclick =
        this.onSetByteLogging_.bind(this, byteLoggingCheckbox);

    this.activelyCapturedCountBox_ = $(ACTIVELY_CAPTURED_COUNT_ID);
    this.passivelyCapturedCountBox_ = $(PASSIVELY_CAPTURED_COUNT_ID);
    $(DELETE_ALL_ID).onclick =
        g_browser.sourceTracker.deleteAllSourceEntries.bind(
            g_browser.sourceTracker);

    $(TIP_ANCHOR_ID).onclick =
        this.toggleCommandLineTip_.bind(this, TIP_DIV_ID);

    this.updateEventCounts_();

    g_browser.sourceTracker.addObserver(this);
  }

  cr.addSingletonGetter(CaptureView);

  CaptureView.prototype = {
    // Inherit the superclass's methods.
    __proto__: superClass.prototype,

    /**
     * Called whenever a new event is received.
     */
    onSourceEntriesUpdated: function(sourceEntries) {
      this.updateEventCounts_();
    },

    /**
     * Toggles the visilibity on the command-line tip.
     */
    toggleCommandLineTip_: function(divId) {
      var n = $(divId);
      var isVisible = n.style.display != 'none';
      setNodeDisplay(n, !isVisible);
      return false;  // Prevent default handling of the click.
    },

    /**
     * Called whenever some log events are deleted.  |sourceIds| lists
     * the source IDs of all deleted log entries.
     */
    onSourceEntriesDeleted: function(sourceIds) {
      this.updateEventCounts_();
    },

    /**
     * Called whenever all log events are deleted.
     */
    onAllSourceEntriesDeleted: function() {
      this.updateEventCounts_();
    },

    /**
     * Called when a log file is loaded, after clearing the old log entries and
     * loading the new ones.  Returns false to indicate the view should
     * be hidden.
     */
    onLoadLogFinish: function(data) {
      return false;
    },

    /**
     * Updates the counters showing how many events have been captured.
     */
    updateEventCounts_: function() {
      this.activelyCapturedCountBox_.textContent =
          g_browser.sourceTracker.getNumActivelyCapturedEvents();
      this.passivelyCapturedCountBox_.textContent =
          g_browser.sourceTracker.getNumPassivelyCapturedEvents();
    },

    /**
     * Depending on the value of the checkbox, enables or disables logging of
     * actual bytes transferred.
     */
    onSetByteLogging_: function(byteLoggingCheckbox) {
      if (byteLoggingCheckbox.checked) {
        g_browser.setLogLevel(LogLevelType.LOG_ALL);
      } else {
        g_browser.setLogLevel(LogLevelType.LOG_ALL_BUT_BYTES);
      }
    }
  };

  return CaptureView;
})();
