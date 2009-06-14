<?php
// $Id$

/**
 * @file
 * Drupal stream wrapper manager
 *
 * Provide a class for managing and querying user defined stream wrappers
 * in PHP. PHP's internal stream_get_wrappers doesn't return the class
 * registered to handle a stream. We need to be able to find the handler
 * for class instantiation.
 *
 * A stream is referenced as: scheme://target
 */

/**
 * Drupal stream wrapper manager class
 */
class DrupalStreamWrapperRegistry {

  // stream wrapper registry
  private static $wrappers = array();

  /**
   * Private constructor to enforce singleton.
   */
  private function __construct() { }

  /**
   * Register a stream wrapper scheme.
   *
   * @see: http://php.net/manual/en/function.stream-wrapper-register.php
   *
   * @param string $scheme
   *   URI scheme.
   * @param string $class
   *   Class name for the stream wrapper.
   * @return bool
   *   result of stream_wrapper_register()
   */
  public static function register($scheme, $class_name) {
    self::$wrappers[$scheme] = $class_name;
    return stream_wrapper_register($scheme, $class_name);
  }

  /**
   * Unregister a stream wrapper.
   *
   * @see: http://php.net/manual/en/function.stream-wrapper-unregister.php
   *
   * @param string $scheme
   *   URI scheme.
   * @return bool
   *   result of stream_wrapper_unregister()
   */
  public static function unregister($scheme) {
    unset(self::$wrappers[$scheme]);
    return stream_wrapper_unregister($scheme);
  }

  /**
   * Return the stream wrapper class name for a given scheme.
   *
   * @param string $scheme
   *   Stream scheme.
   * @return mixed
   *   Return string if a scheme has a registered handler, or FALSE.
   */
  public static function getClassName($scheme) {
    if (empty(self::$wrappers[$scheme])) {
      return FALSE;
    }

    return self::$wrappers[$scheme];
  }

  /**
   * Return the stream class name for a given scheme.
   *
   * @param string $scheme
   *   Stream scheme.
   * @return mixed
   *   Return string if a scheme has a registered handler, or FALSE.
   */
  function scheme($class) {
    return array_search(self::$wrappers, $class);
  }

  /**
   * Return the entire Drupal stream wrapper registry.
   *
   * @return array
   */
  function wrappers() {
    return self::$wrappers;
  }

  /**
   * Get the scheme of a URI (stream).
   *
   * A stream is referenced as scheme://target.
   *
   * @param $uri
   *   A stream, referenced as scheme://target
   * @return mixed
   *   A string containing the name of the scheme, or FALSE if none.
   *   For example, the URI public://example.txt would return public.
   */
  public static function getStreamScheme($uri) {
    $data = explode('://', $uri, 2);
    return count($data) == 2 ? $data[0] : FALSE;
  }

  /**
   * Get reference to stream wrapper class responsible for given URI (stream).
   *
   * The scheme determines the stream wrapper class that should be
   * used by consulting the stream wrapper registry.
   *
   * @param $uri
   *   A stream, referenced as scheme://target
   * @return mixed
   *   Returns a new stream wrapper object appropriate for the given URI.
   *   For example, a URI of public://example.txt would return a new
   *   private stream wrapper object (DrupalPrivateStreamWrapper).
   *   FALSE is returned if no registered handler could be found.
   */
  public static function getInstance($uri) {

    $class = self::getClassName(self::getStreamScheme($uri));
    if (class_exists($class)) {
      return new $class;
    }
    else {
      return FALSE;
    }
  }

}
