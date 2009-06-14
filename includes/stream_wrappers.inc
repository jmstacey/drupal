<?php
// $Id$

/**
 * @file
 * Drupal stream wrapper interface.
 *
 * Provides a Drupal interface and classes to implement PHP stream wrappers for
 * public, private, and temporary files.
 */


/**
 * Generic PHP stream wrapper interface.
 *
 * @see: http://www.php.net/manual/en/class.streamwrapper.php
 */
interface StreamWrapperInterface {
  public function stream_open($uri, $mode, $options, &$opened_url);
  public function stream_close();
  public function stream_lock($operation);
  public function stream_read($count);
  public function stream_write($data);
  public function stream_eof();
  public function stream_seek($offset, $whence);
  public function stream_flush();
  public function stream_tell();
  public function stream_stat();
  public function unlink($uri);
  public function rename($from_uri, $to_uri);
  public function mkdir($uri, $mode, $options);
  public function rmdir($uri, $options);
  public function url_stat($uri, $flags);
  public function dir_opendir($uri, $options);
  public function dir_readdir();
  public function dir_rewinddir();
  public function dir_closedir();
}


/**
 * Drupal stream wrapper extension.
 *
 * Extend the StreamWrapperInterface with methods expected by
 * Drupal stream wrapper classes.
 */
interface DrupalStreamWrapperInterface extends StreamWrapperInterface {

  /**
   * Return an absolute stream resource URL.
   *
   * @param $uri
   *   Stream wrapper resource url.
   * @return string
   */
  static function getInternalUri($uri);

  /**
   * Return the HTML accessible URL for a resource.
   *
   * @param $uri
   *   Stream wrapper resource url.
   * @return string
   */
  static function getExternalUrl($uri);

  /**
   * Return the mime type of a resource.
   *
   * @param $uri
   *   Stream wrapper resource url.
   * @return string
   */
  static function getMimeType($uri);

  /**
   * Change permissions of stream.
   *
   * PHP lacks this functionality and it is not part of the official
   * stream wrapper interface. This is a custom implementation for
   * Drupal.
   *
   * @param string $uri
   * @param mixed $mode
   * @return bool
   *   Returns TRUE on success or FALSE on failure.
   */
  static function chmod($uri, $mode);

  /**
   * Returns canonicalized absolute pathname.
   *
   * Implementation placeholder. PHP's realpath does not support
   * stream wrappers. We provide this as a default so that
   * individual wrappers may implement their own solutions.
   * The base class will always return FALSE.
   *
   * @param string $uri
   * @return mixed
   *   A string with absolute pathname on success (implemented
   *   by core wrappers), or FALSE on failure or the registered
   *   wrapper does not provide an implementation.
   */
  static function realpath($uri);
}


/**
 * Drupal stream wrapper base class for local files.
 *
 * This class provides a complete stream wrapper implementation. It passes
 * incoming URI's through an interpolation method and then recursively calls
 * the invoking PHP filesystem function.
 *
 * DrupalLocalStreamWrapper implementations need to override at least the
 * getInternalUri() method to rewrite the URI before is it passed back to the
 * calling function.
 */
abstract class DrupalLocalStreamWrapper implements DrupalStreamWrapperInterface {

  private $handle = NULL;

  /**
   * Where are the files located?
   *
   * @return
   *   String specifying the path.
   */
  abstract static function getDirectoryPath();

  /**
   * Interpolate the URI path, adding the base path from $this->getDirectoryPath().
   */
  static function getInternalUri($uri) {
    return realpath(self::getDirectoryPath()) . str_replace('/..','', parse_url($uri, PHP_URL_PATH));
  }

  static function getExternalUrl($uri) {
    return $uri;
  }

  static function getMimeType($uri) {
    return file_get_mimetype(basename($uri));
  }

  static function chmod($uri, $mode) {
    return @chmod($uri, $mode);
  }

  static function realpath($uri) {
    return @realpath(self::getInternalUri($uri));
  }

  /**
   * Support for fopen(), file_get_contents(), file_put_contents() etc.
   *
   * @see http://php.net/manual/en/streamwrapper.stream-open.php
   *
   * @param $path
   *   A string containing the path to the file to open.
   * @param $mode
   *   The file mode ("r", "wb" etc.).
   * @param $options
   *   A bit mask of STREAM_USE_PATH and STREAM_REPORT_ERRORS.
   * @param &$opened_path
   *   A string containing the path actually opened.
   * @return bool
   *   TRUE if file was opened successfully.
   */
  public function stream_open($uri, $mode, $options, &$opened_url) {
    $uri = $this->getInternalUri($uri);
    $this->handle = ($options & STREAM_REPORT_ERRORS) ? fopen($uri, $mode) : @fopen($uri, $mode);

    if ((bool)$this->handle && $options & STREAM_USE_PATH) {
      $opened_url = $uri;
    }

    return (bool)$this->handle;
  }

  /**
   * Support for flock().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-lock.php
   *
   * @param int $operation
   * @return bool
   *   Always returns TRUE.
   */
  function stream_lock($operation) {
    if (in_array($operation, array(LOCK_SH, LOCK_EX, LOCK_UN, LOCK_NB))) {
      return flock($this->handle, $operation);
    }

    return TRUE;
  }

  /**
   * Support for fread(), file_get_contents() etc.
   *
   * @see http://php.net/manual/en/streamwrapper.stream-read.php
   *
   * @param $count
   *   Maximum number of bytes to be read.
   * @return
   *   The string that was read, or FALSE in case of an error.
   */
  public function stream_read($count) {
    return fread($this->handle, $count);
  }

  /**
   * Support for fwrite(), file_put_contents() etc.
   *
   * @see http://php.net/manual/en/streamwrapper.stream-write.php
   *
   * @param $data
   *   The string to be written.
   * @return int
   *   The number of bytes written.
   */
  public function stream_write($data) {
    return fwrite($this->handle, $data);
  }

  /**
   * Support for feof().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-eof.php
   *
   * @return bool
   *   TRUE if end-of-file has been reached.
   */
  public function stream_eof() {
    return feof($this->handle);
  }

  /**
   * Support for fseek().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-seek.php
   *
   * @param $offset
   *   The byte offset to got to.
   * @param $whence
   *   SEEK_SET, SEEK_CUR, or SEEK_END.
   * @return
   *   TRUE on success
   */
  public function stream_seek($offset, $whence) {
    return fseek($this->handle, $offset, $whence);
  }

  /**
   * Support for fflush().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-flush.php
   *
   * @return
   *   TRUE if data was successfully stored (or there was no data to store).
   */
  public function stream_flush() {
    return fflush($this->handle);
  }

  /**
   * Support for ftell().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-tell.php
   *
   * @return
   *   The current offset in bytes from the beginning of file.
   */
  public function stream_tell() {
    return ftell($this->handle);
  }

  /**
   * Support for fstat().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-stat.php
   *
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   */
  public function stream_stat() {
    return fstat($this->handle);
  }

  /**
   * Support for fclose().
   *
   * @see http://php.net/manual/en/streamwrapper.stream-close.php
   *
   * @return
   *   TRUE if stream was successfully closed.
   */
  public function stream_close() {
    return fclose($this->handle);
  }

  /**
   * Support for unlink().
   *
   * @see http://php.net/manual/en/streamwrapper.unlink.php
   *
   * @param $uri
   *   A string containing the uri to the resource to delete.
   * @return
   *   TRUE if resource was successfully deleted.
   */
  public function unlink($uri) {
    return unlink($this->getInternalUri($uri));
  }

  /**
   * Support for rename().
   *
   * @see http://php.net/manual/en/streamwrapper.rename.php
   *
   * @param $from_uri,
   *   The uri to the file to rename.
   * @param $to_uri
   *   The new uri for file.
   * @return
   *   TRUE if file was successfully renamed.
   */
  public function rename($from_uri, $to_uri) {
    return rename($this->getInternalUri($from_uri), $this->getInternalUri($to_uri));
  }

  /**
   * Support for mkdir().
   *
   * @see http://php.net/manual/en/streamwrapper.mkdir.php
   *
   * @param $uri
   *   A string containing the url to the directory to create.
   * @param $mode
   *   Permission flags - see mkdir().
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS and STREAM_MKDIR_RECURSIVE.
   * @return
   *   TRUE if directory was successfully created.
   */
  public function mkdir($uri, $mode, $options) {
    $recursive = (bool)($options & STREAM_MKDIR_RECURSIVE);
    if ($options & STREAM_REPORT_ERRORS) {
      return mkdir($this->getInternalUri($uri), $mode, $recursive);
    }
    else {
      return @mkdir($this->getInternalUri($uri), $mode, $recursive);
    }
  }

  /**
   * Support for rmdir().
   *
   * @see http://php.net/manual/en/streamwrapper.rmdir.php
   *
   * @param $uri
   *   A string containing the url to the directory to delete.
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS.
   * @return
   *   TRUE if directory was successfully removed.
   */
  public function rmdir($uri, $options) {
    if ($options & STREAM_REPORT_ERRORS) {
      return rmdir($this->getInternalUri($uri));
    }
    else {
      return @rmdir($this->getInternalUri($uri));
    }
  }

  /**
   * Support for stat().
   *
   * @see http://php.net/manual/en/streamwrapper.url-stat.php
   *
   * @param $uri
   *   A string containing the url to get information about.
   * @param $flags
   *   A bit mask of STREAM_URL_STAT_LINK and STREAM_URL_STAT_QUIET.
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   */
  public function url_stat($uri, $flags) {
    if ($flags & STREAM_URL_STAT_QUIET) {
      return @stat($this->getInternalUri($uri));
    }
    else {
      return stat($this->getInternalUri($uri));
    }
  }

  /**
   * Support for opendir().
   *
   * @see http://php.net/manual/en/streamwrapper.dir-opendir.php
   *
   * @param $uri
   *   A string containing the url to the directory to open.
   * @param $options
   *   Unknown (parameter is not documented in PHP Manual).
   * @return
   *   TRUE on success.
   */
  public function dir_opendir($uri, $options) {
    $this->handle = opendir($this->getInternalUri($uri));

    return (bool)$this->handle;
  }

  /**
   * Support for readdir().
   *
   * @see http://php.net/manual/en/streamwrapper.dir-readdir.php
   *
   * @return
   *   The next filename, or FALSE if there are no more files in the directory.
   */
  public function dir_readdir() {
    return readdir($this->handle);
  }

  /**
   * Support for rewinddir().
   *
   * @see http://php.net/manual/en/streamwrapper.dir-rewinddir.php
   *
   * @return
   *   TRUE on success.
   */
  public function dir_rewinddir() {
    return rewinddir($this->handle);
  }

  /**
   * Support for closedir().
   *
   * @see http://php.net/manual/en/streamwrapper.dir-closedir.php
   *
   * @return
   *   TRUE on success.
   */
  public function dir_closedir() {
    return closedir($this->handle);
  }
}


/**
 * Drupal public (public://) stream wrapper class.
 *
 * Provides support for storing publicly accessible
 * files with the Drupal file interface.
 */
class DrupalPublicStreamWrapper extends DrupalLocalStreamWrapper {

  static function getDirectoryPath() {
    return variable_get('stream_public_path', 'sites/default/files');
  }

  /**
   * Override getExternalUrl().
   *
   * Return the HTML URI of a public file.
   */
  static function getExternalUrl($uri) {
    $path = str_replace('\\', '/', parse_url($uri, PHP_URL_PATH));
    return $GLOBALS['base_url'] . '/' . self::getDirectoryPath() . '/' . $path;
  }
}


/**
 * Drupal private (private://) stream wrapper class.
 *
 * Provides support for storing privately accessible
 * files with the Drupal file interface.
 *
 * Extends DrupalPublicStreamWrapper.
 */
class DrupalPrivateStreamWrapper extends DrupalLocalStreamWrapper {

  static function getDirectoryPath() {
    return variable_get('stream_private_path', 'sites/default/files-private');
  }

  /**
   * Override getHtmlUri.
   *
   * Return the HTML URI of a private file.
   */
  static function getExternalUrl($uri) {
    $path = str_replace('\\', '/', parse_url($uri, PHP_URL_PATH));
    return url('system/files/' . parse_url($uri, PHP_URL_PATH), array('absolute' => TRUE));
  }
}


/**
 * Drupal temp (temp://) stream wrapper class.
 *
 * Provides support for storing temporarily accessible
 * files with the Drupal file interface.
 *
 * Extends DrupalPublicStreamWrapper.
 */
class DrupalTempStreamWrapper extends DrupalLocalStreamWrapper {

  static function getDirectoryPath() {
    return variable_get('file_directory_temp', 'sites/default/files/tmp');
  }

  /**
   * Override getHtmlUri.
   *
   * Return the HTML URI of a temporary file.
   */
  static function getExternalUrl($uri) {
// TODO: This needs it's own separate menu handler.
    $path = str_replace('\\', '/', parse_url($uri, PHP_URL_PATH));
    return url('system/files/' . $path, array('absolute' => TRUE));
  }
}
