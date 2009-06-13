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

  private $wrappers = array(); // stream wrapper registry

  /**
   * Private constructor to enforce singleton.
   */
  private function __construct() { }

  /**
   * Load the singleton instance of the stream_wrapper_manager.
   *
   * @return object
   *   A stream_wrapper_manager instance.
   */
  public static function singleton() {
    static $instance = NULL;
    if (is_null($instance)) {
      $instance = new DrupalStreamWrapperRegistry();
    }
    return $instance;
  }

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
  function register($scheme, $class_name) {
    $this->wrappers[$scheme] = $class_name;

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
  function unregister($scheme) {
    if (stream_wrapper_unregister($scheme)) {
      unset(self::$this->wrappers[$scheme]);
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Return the stream wrapper class name for a given scheme.
   *
   * @param string $scheme
   *   Stream scheme.
   * @return mixed
   *   Return string if a scheme has a registered handler, or FALSE.
   */
  function class_name($scheme) {
    if (empty(self::$this->wrappers[$scheme])) {
      return FALSE;
    }

    return self::$this->wrappers[$scheme];
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
    return array_search(self::$this->wrappers, $class);
  }

  /**
   * Return the entire Drupal stream wrapper registry.
   *
   * @return array
   */
  function wrappers() {
    return self::$this->wrappers;
  }
}
