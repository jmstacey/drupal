<?php
// $Id$

/**
 * @file
 * Temp (temp://) stream wrapper class.
 *
 * Provides support for storing temporarily accessible
 * files with the Drupal file interface.
 */

/**
 * Drupal temp stream wrapper class.
 *
 * Extends drupal_public_stream_wrapper.
 */
class drupal_temp_stream_wrapper extends drupal_public_stream_wrapper {

  private $path_key     = 'file_directory_temp';
  private $path_default = 'sites/default/files/tmp';

  /**
   * Override html_uri.
   *
   * Return the HTML URI of a private file.
   */
  function html_uri($uri) {
    $basepath = variable_get($this->path_key, $this->path_default);

    return url('system/files/' . parse_url($uri, PHP_URL_PATH), array('absolute' => TRUE));
  }
}
