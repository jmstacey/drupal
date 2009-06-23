<?php
// $Id$

/**
 * @file
 * Drupal stream wrapper registry.
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

  /**
   * Array mapping schemes to class names.
   *
   * @var Array
   */
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
   *   A stream, referenced as scheme://target.
   * @return mixed
   *   A string containing the name of the scheme, or FALSE if none.
   *   For example, the URI public://example.txt would return public.
   */
  public static function getStreamScheme($uri) {
    $data = explode('://', $uri, 2);
    return count($data) == 2 ? $data[0] : FALSE;
  }

  /**
   * Is the provided scheme valid?
   *
   * Confirms that there is a registered stream handler for the
   * provided scheme and that it is callable. This is usefule if
   * you want to confirm a valid scheme without creating a new instance
   * of the registered handler.
   *
   * A stream is referenced as scheme://target.
   *
   * @code
   *   // Return 'public'
   *   DrupalStreamWrapperRegistry::getStreamValidate('public://example.txt');
   *
   *   // Return FALSE unless there is a registered wrapper for 'foobar'
   *   DrupalStreamWrapperRegistry::getStreamValidate('foobar://exampl.txt');
   *
   * @param $uri
   *   A stream, referenced as scheme://target.
   * @return mixed
   *   Returns a string containing the name of a validated stream.
   *   Returns false if the URI does not contain a scheme or the scheme
   *   does not have a registered handler.
   */
  public static function getValidStreamScheme($uri) {
    $data = explode('://', $uri, 2);

    // URI does not contain a scheme
    if (count($data) != 2) {
      return FALSE;
    }

    // Does the scheme have a registered handler that is callable?
    $class = self::getClassName($data[0]);
    if (class_exists($class)) {
      return $data[0];
    }
    else {
      return FALSE;
    }
  }

  /**
   * Get the targat of a URI (stream).
   *
   * A stream is referenced as scheme://target.
   *
   * @param $uri
   *   A stream, referenced as scheme://target
   * @return mixed
   *   A string containing the target (path), or FALSE if none.
   *   For example, the URI public://sample/test.txt would return 
   *   sample/test.txt
   */
  public static function getStreamTarget($uri) {
    $data = explode('://', $uri, 2);
    // $data[0]://$data[1]
    // scheme://target

    // Remove erroneous beginning forward slash
    $data[1] = ltrim($data[1], '\/');

    return count($data) == 2 ? $data[1] : FALSE;
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
  public static function getInstanceByUri($uri) {

    return self::getInstanceByScheme(self::getStreamScheme($uri));
  }

  /**
   * Get reference to stream wrapper class responsible for given scheme.
   *
   * The scheme determines the stream wrapper class that should be
   * used by consulting the stream wrapper registry.
   *
   * @param $scheme
   * @return mixed
   *   Returns a new stream wrapper object appropriate for the given $scheme.
   *   For example, for the public scheme a stream wrapper object
   *   (DrupalPublicStreamWrapper).
   *   FALSE is returned if no registered handler could be found.
   */
  public static function getInstanceByScheme($scheme) {

    $class = self::getClassName($scheme);
    if (class_exists($class)) {
      return new $class;
    }
    else {
      return FALSE;
    }
  }

}
