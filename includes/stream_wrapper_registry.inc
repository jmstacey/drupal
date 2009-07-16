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
   * Registers a stream wrapper scheme.
   *
   * @param string $scheme
   *   URI scheme.
   * @param string $class
   *   Class name for the stream wrapper.
   * @return bool
   *   result of stream_wrapper_register()
   * @see http://php.net/manual/en/function.stream-wrapper-register.php
   */
  public static function register($scheme, $class_name) {
    self::$wrappers[$scheme] = $class_name;
    return stream_wrapper_register($scheme, $class_name);
  }

  /**
   * Unregisters a stream wrapper.
   *
   * @param string $scheme
   *   URI scheme.
   * @return bool
   *   result of stream_wrapper_unregister()
   * @see http://php.net/manual/en/function.stream-wrapper-unregister.php
   */
  public static function unregister($scheme) {
    unset(self::$wrappers[$scheme]);
    return stream_wrapper_unregister($scheme);
  }

  /**
   * Returns the entire Drupal stream wrapper registry.
   *
   * @return array
   */
  public function wrappers() {
    return self::$wrappers;
  }

  /**
   * Returns the stream wrapper class name for a given scheme.
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
   * Returns the scheme of a stream [URI].
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
   * Asserts that the scheme is valid.
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
   *   DrupalStreamWrapperRegistry::getValidStreamScheme('public://example.txt');
   *
   *   // Return FALSE unless there is a registered wrapper for 'foobar'
   *   DrupalStreamWrapperRegistry::getValidStreamScheme('foobar://example.txt');
   *
   * @param $uri
   *   A stream, referenced as scheme://target.
   * @return mixed
   *   Returns a string containing the name of a validated stream.
   *   Returns false if the URI does not contain a scheme or the scheme
   *   does not have a registered handler.
   */
  public static function getValidStreamScheme($uri) {
    if (!$scheme = self::getStreamScheme($uri)) {
      // URI doesn't even contain a scheme
      return FALSE;
    }

    // Does the scheme have a registered handler that is callable?
    $class = self::getClassName($scheme);
    if (class_exists($class)) {
      return $scheme;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Returns the target of a stream [URI].
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

    if (count($data) != 2) {
      return FALSE;
    }

    // Remove erroneous beginning forward slash
    $data[1] = ltrim($data[1], '\/');

    return $data[1];
  }

  /**
   * Returns a reference to the stream wrapper class responsible for a given URI (stream).
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
    $class = self::getClassName(self::getStreamScheme($uri));
    if (class_exists($class)) {
      $instance = new $class;
      $instance->__set('uri', $uri);
      return $instance;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Returns a reference to the stream wrapper class responsible for a given scheme.
   *
   * This helper method returns a stream instance using a scheme. That is, the
   * passed string does not container a '://'. For example, 'public' is a scheme 
   * but 'public://' is a URI (stream). This is because the later contains both 
   * a scheme and target despite target being empty.
   *
   * Note: the instance URI will be initialized to 'scheme://' so that you can
   * make the customary method calls as if you had retrieved an instance by URI.
   *
   * @param $scheme
   *   If the stream was 'public://target', 'public' would be the scheme.
   * @return mixed
   *   Returns a new stream wrapper object appropriate for the given $scheme.
   *   For example, for the public scheme a stream wrapper object
   *   (DrupalPublicStreamWrapper).
   *   FALSE is returned if no registered handler could be found.
   */
  public static function getInstanceByScheme($scheme) {
    return self::getInstanceByUri($scheme . '://');
  }

  /**
   * Normalizes URI by making it syntactically correct.
   *
   * A stream is referenced as scheme://target.
   *
   * The following actions are taken:
   * - Replace ':///' with '://'
   * - Remove trailing slashes on target.
   *
   * @param string &$uri
   *   String reference containing the URI to normalize.
   */
  public static function normalizeUri(&$uri) {
    if ($scheme = self::getValidStreamScheme($uri)) {
      $target = self::getStreamTarget($uri);

      // Remove all occurrences of the wrapper's directory path.
      $directory_path = self::getInstanceByScheme($scheme)->getDirectoryPath();
      $target         = str_replace($directory_path, '', $target);

      // Trim erroneous leading slashes from target.
      $uri = $scheme . '://' . ereg_replace('^[/\\]*', '', $target);
    }
  }

}
