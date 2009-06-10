<?php
// $Id$

/**
 * @file
 * Private (private://) stream wrapper class.
 *
 * Provides support for storing privately accessible 
 * files with the Drupal file interface.
 */
 
/**
 * Drupal private stream wrapper class.
 *
 * Extends drupal_public_stream_wrapper.
 */
class drupal_private_stream_wrapper extends drupal_public_stream_wrapper {
  
  private $path_key     = 'stream_private_path';
  private $path_default = 'sites/default/files-private';

  /**
   * Override html_uri.
   *
   * Return the HTML URI of a private file.
   */
  function html_uri($url) {
    $basepath = variable_get($this->path_key, $this->path_default);
    
    return url('system/files/' . parse_url($url, PHP_URL_PATH), array('absolute' => TRUE));
  }
}
