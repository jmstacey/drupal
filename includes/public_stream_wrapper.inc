<?php
// $Id$

/**
 * @file
 * Public (public://) stream wrapper class.
 *
 * Provides support for storing publicly accessible 
 * files with the Drupal file interface.
 */
 
/**
 * Drupal public stream wrapper class.
 *
 * Extends drupal_stream_wrapper.
 */
class drupal_public_stream_wrapper extends drupal_stream_wrapper {

  // A handle to the file opened by stream_open().
  private $path_key     = 'stream_public_path';
  private $path_default = 'sites/default/files';

  /**
   * Override interpolate_uri().
   *
   * Interpolate the URI path, adding the public files path.
   */
  function interpolate_uri($uri) {
    $basepath = variable_get($this->path_key, $this->path_default);

    // just in case stream_public_path is s3://, ftp://, etc. Don't call PHP's
    // drupal_realpath().
    if (parse_url($basepath, PHP_URL_SCHEME)) {
      $path = $basepath . parse_url($uri, PHP_URL_PATH);;
    }
    else {
      // interpolate relative paths for basepath, and strip relative paths from
      // url path.
      $path = drupal_realpath($basepath) . str_replace('/..','', parse_url($uri, PHP_URL_PATH));
    }
    
    return $path;
  }

  /**
   * Override html_uri().
   *
   * Return the HTML URI of a public file.
   */
  function html_uri($uri) {
    $basepath = variable_get($this->path_key, $this->path_default);
    $path     = parse_url($uri, PHP_URL_PATH);
    
    return $GLOBALS['base_url'] . '/' . $basepath . '/' . str_replace('\\', '/', $path);
  }

  /**
   * Override mime().
   *
   * Return the mime type of a file.
   */
  function mime($uri) {
    return file_get_mimetype(basename($uri));
  }
  
  /**
   * Override stream_realpath().
   *
   * Interpolates the given URI and then passes it to PHP's realpath.
   */
   function stream_realpath($uri) {
     return realpath($this->interpolate_uri($uri));
   }
  
}
