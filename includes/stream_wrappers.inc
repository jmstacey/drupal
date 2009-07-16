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
 *  The scheme for the public stream wrapper.
 */
define('SCHEME_PUBLIC', 'public://');

/**
 *  The scheme for the private stream wrapper.
 */
define('SCHEME_PRIVATE', 'private://');

/**
 *  The scheme for the temporary stream wrapper.
 */
define('SCHEME_TEMP', 'temp://');

/**
 * Generic PHP stream wrapper interface.
 *
 * @see http://www.php.net/manual/en/class.streamwrapper.php
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
   * Returns an absolute stream resource URL for internal purposes. 
   *
   * In the case of the three local wrappers (public, private, temp)
   * drupal_realpath() should be used for security reasons.
   *
   * For example, the internal path of public://foobar.txt might be
   * sites/default/files/foobar.txt. Following this call with
   * realpath() will return an absolute path such as
   * /Users/foo/Sites/drupal/sites/default/files/foobar.txt.
   *
   * @return string
   *   Returns cononcialized absolute pathname for internal purposes.
   * @see realpath()
   * @see drupal_realpath()
   */
  public function getInternalUri();

  /**
   * Returns a web accessible URL for the resource.
   *
   * This function should return a URL that can be embedded in a web page
   * and accessed from a browser. For example, the external URL of
   * youtube://xIpLd0WQKCY might be http://www.youtube.com/watch?v=xIpLd0WQKCY.
   *
   * @return string
   */
  public function getExternalUrl();

  /**
   * Returns the MIME type of the resource.
   *
   * @param array $mapping
   *   An optional map of extensions to their mimetypes, in the form:
   *    - 'mimetypes': a list of mimetypes, keyed by an identifier,
   *    - 'extensions': the mapping itself, an associative array in which
   *      the key is the extension and the value is the mimetype identifier.
   * @return string
   */
  public function getMimeType($mapping = NULL);

  /**
   * Changes permissions of the resource.
   *
   * PHP lacks this functionality and it is not part of the official
   * stream wrapper interface. This is a custom implementation for
   * Drupal.
   *
   * @param mixed $mode
   * @return bool
   *   Returns TRUE on success or FALSE on failure.
   */
  public function chmod($mode);

  /**
   * Returns canonical, absolute path of the resource.
   *
   * Implementation placeholder. PHP's realpath() does not support
   * stream wrappers. We provide this as a default so that
   * individual wrappers may implement their own solutions.
   * The base class will always return FALSE.
   *
   * @return mixed
   *   A string with absolute pathname on success (implemented
   *   by core wrappers), or FALSE on failure or the registered
   *   wrapper does not provide an implementation.
   */
  public function realpath();
}


/**
 * Drupal stream wrapper base class for local files.
 *
 * This class provides a complete stream wrapper implementation. URIs such as
 * public://example.txt are expanded to a normal filesystem path such as
 * sites/default/files/example.txt and then PHP filesystem functions are invoked.
 *
 * DrupalLocalStreamWrapper implementations need to override at least the
 * getInternalUri() method to rewrite the URI before is it passed back to the
 * calling function.
 */
abstract class DrupalLocalStreamWrapper implements DrupalStreamWrapperInterface {

  /**
   * Stream context resource.
   *
   * @var Resource
   */
  public $context;

  /**
   * A generic resource handle.
   *
   * @var Resource
   */
  public $handle = NULL;

  /**
   * Instance URI (stream).
   *
   * A stream is referenced as scheme://target
   *
   * @var String
   */
  public $uri;

  /**
   * Gets the path that the wrapper is responsible for.
   *
   * @return
   *   String specifying the path.
   */
  abstract function getDirectoryPath();

  /**
   * Base implementation of getInternalUri().
   */
  function getInternalUri() {
    return $this->getDirectoryPath() . '/' . DrupalStreamWrapperRegistry::getStreamTarget($this->uri);
  }

  /**
   * Base implementation of getExternalUrl().
   */
  function getExternalUrl() {
    return $this->uri;
  }

  /**
   * Base implementation of getMimeType().
   */
  function getMimeType($mapping = NULL) {
    if (!isset($mapping)) {
      $mapping = variable_get('mime_extension_mapping', NULL);
      if (!isset($mapping) && drupal_function_exists('file_default_mimetype_mapping')) {
        // The default file map, defined in file.mimetypes.inc is quite big.
        // We only load it when necessary.
        $mapping = file_default_mimetype_mapping();
      }
    }

    $extension = '';
    $file_parts = explode('.', basename($this->uri));

    // Remove the first part: a full filename should not match an extension.
    array_shift($file_parts);

    // Iterate over the file parts, trying to find a match.
    // For my.awesome.image.jpeg, we try:
    //   - jpeg
    //   - image.jpeg, and
    //   - awesome.image.jpeg
    while ($additional_part = array_pop($file_parts)) {
      $extension = $additional_part . ($extension ? '.' . $extension : '');
      if (isset($mapping['extensions'][$extension])) {
        return $mapping['mimetypes'][$mapping['extensions'][$extension]];
      }
    }

    return 'application/octet-stream';
  }

  /**
   * Base implementation of chmod().
   */
  function chmod($mode) {
    return @chmod($this->realpath(), $mode);
  }

  /**
   * Base implementaiton of realpath().
   */
  function realpath() {
    return @realpath($this->getDirectoryPath() . '/' . DrupalStreamWrapperRegistry::getStreamTarget($this->uri));
  }

  /**
   * Support for fopen(), file_get_contents(), file_put_contents() etc.
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
   * @see http://php.net/manual/en/streamwrapper.stream-open.php
   */
  public function stream_open($uri, $mode, $options, &$opened_url) {
    $this->uri = $uri;
    $uri = $this->getInternalUri();
    $this->handle = ($options & STREAM_REPORT_ERRORS) ? fopen($uri, $mode) : @fopen($uri, $mode);

    if ((bool)$this->handle && $options & STREAM_USE_PATH) {
      $opened_url = $uri;
    }

    return (bool)$this->handle;
  }

  /**
   * Support for flock().
   *
   * @param int $operation
   * @return bool
   *   Always returns TRUE.
   * @see http://php.net/manual/en/streamwrapper.stream-lock.php
   */
  public function stream_lock($operation) {
    if (in_array($operation, array(LOCK_SH, LOCK_EX, LOCK_UN, LOCK_NB))) {
      return flock($this->handle, $operation);
    }

    return TRUE;
  }

  /**
   * Support for fread(), file_get_contents() etc.
   *
   * @param $count
   *   Maximum number of bytes to be read.
   * @return
   *   The string that was read, or FALSE in case of an error.
   * @see http://php.net/manual/en/streamwrapper.stream-read.php
   */
  public function stream_read($count) {
    return fread($this->handle, $count);
  }

  /**
   * Support for fwrite(), file_put_contents() etc.
   *
   * @param $data
   *   The string to be written.
   * @return int
   *   The number of bytes written.
   * @see http://php.net/manual/en/streamwrapper.stream-write.php
   */
  public function stream_write($data) {
    return fwrite($this->handle, $data);
  }

  /**
   * Support for feof().
   *
   * @return bool
   *   TRUE if end-of-file has been reached.
   * @see http://php.net/manual/en/streamwrapper.stream-eof.php
   */
  public function stream_eof() {
    return feof($this->handle);
  }

  /**
   * Support for fseek().
   *
   * @param $offset
   *   The byte offset to got to.
   * @param $whence
   *   SEEK_SET, SEEK_CUR, or SEEK_END.
   * @return
   *   TRUE on success
   * @see http://php.net/manual/en/streamwrapper.stream-seek.php
   */
  public function stream_seek($offset, $whence) {
    return fseek($this->handle, $offset, $whence);
  }

  /**
   * Support for fflush().
   *
   * @return
   *   TRUE if data was successfully stored (or there was no data to store).
   * @see http://php.net/manual/en/streamwrapper.stream-flush.php
   */
  public function stream_flush() {
    return fflush($this->handle);
  }

  /**
   * Support for ftell().
   *
   * @return
   *   The current offset in bytes from the beginning of file.
   * @see http://php.net/manual/en/streamwrapper.stream-tell.php
   */
  public function stream_tell() {
    return ftell($this->handle);
  }

  /**
   * Support for fstat().
   *
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   * @see http://php.net/manual/en/streamwrapper.stream-stat.php
   */
  public function stream_stat() {
    return fstat($this->handle);
  }

  /**
   * Support for fclose().
   *
   * @return
   *   TRUE if stream was successfully closed.
   * @see http://php.net/manual/en/streamwrapper.stream-close.php
   */
  public function stream_close() {
    return fclose($this->handle);
  }

  /**
   * Support for unlink().
   *
   * @param $uri
   *   A string containing the uri to the resource to delete.
   * @return
   *   TRUE if resource was successfully deleted.
   * @see http://php.net/manual/en/streamwrapper.unlink.php
   */
  public function unlink($uri) {
    $this->uri = $uri;
    return unlink($this->getInternalUri());
  }

  /**
   * Support for rename().
   *
   * @param $from_uri,
   *   The uri to the file to rename.
   * @param $to_uri
   *   The new uri for file.
   * @return
   *   TRUE if file was successfully renamed.
   * @see http://php.net/manual/en/streamwrapper.rename.php
   */
  public function rename($from_uri, $to_uri) {
    return rename($this->getInternalUri($from_uri), $this->getInternalUri($to_uri));
  }

  /**
   * Support for mkdir().
   *
   * @param $uri
   *   A string containing the url to the directory to create.
   * @param $mode
   *   Permission flags - see mkdir().
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS and STREAM_MKDIR_RECURSIVE.
   * @return
   *   TRUE if directory was successfully created.
   * @see http://php.net/manual/en/streamwrapper.mkdir.php
   */
  public function mkdir($uri, $mode, $options) {
    $this->uri = $uri;
    $recursive = (bool)($options & STREAM_MKDIR_RECURSIVE);
    if ($options & STREAM_REPORT_ERRORS) {
      return mkdir($this->getInternalUri(), $mode, $recursive);
    }
    else {
      return @mkdir($this->getInternalUri(), $mode, $recursive);
    }
  }

  /**
   * Support for rmdir().
   *
   * @param $uri
   *   A string containing the url to the directory to delete.
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS.
   * @return
   *   TRUE if directory was successfully removed.
   * @see http://php.net/manual/en/streamwrapper.rmdir.php
   */
  public function rmdir($uri, $options) {
    $this->uri = $uri;
    if ($options & STREAM_REPORT_ERRORS) {
      return rmdir($this->getInternalUri());
    }
    else {
      return @rmdir($this->getInternalUri());
    }
  }

  /**
   * Support for stat().
   *
   * @param $uri
   *   A string containing the url to get information about.
   * @param $flags
   *   A bit mask of STREAM_URL_STAT_LINK and STREAM_URL_STAT_QUIET.
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   * @see http://php.net/manual/en/streamwrapper.url-stat.php
   */
  public function url_stat($uri, $flags) {
    $this->uri = $uri;
    if ($flags & STREAM_URL_STAT_QUIET) {
      return @stat($this->getInternalUri());
    }
    else {
      return stat($this->getInternalUri());
    }
  }

  /**
   * Support for opendir().
   *
   * @param $uri
   *   A string containing the url to the directory to open.
   * @param $options
   *   Unknown (parameter is not documented in PHP Manual).
   * @return
   *   TRUE on success.
   * @see http://php.net/manual/en/streamwrapper.dir-opendir.php
   */
  public function dir_opendir($uri, $options) {
    $this->uri = $uri;
    $this->handle = opendir($this->getInternalUri());

    return (bool)$this->handle;
  }

  /**
   * Support for readdir().
   *
   * @return
   *   The next filename, or FALSE if there are no more files in the directory.
   * @see http://php.net/manual/en/streamwrapper.dir-readdir.php
   */
  public function dir_readdir() {
    return readdir($this->handle);
  }

  /**
   * Support for rewinddir().
   *
   * @return
   *   TRUE on success.
   * @see http://php.net/manual/en/streamwrapper.dir-rewinddir.php
   */
  public function dir_rewinddir() {
    return rewinddir($this->handle);
  }

  /**
   * Support for closedir().
   *
   * @return
   *   TRUE on success.
   * @see http://php.net/manual/en/streamwrapper.dir-closedir.php
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

  /**
   * Implements abstract public function getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('stream_public_path', 'sites/default/files');
  }

  /**
   * Overrides getExternalUrl().
   *
   * Return the HTML URI of a public file.
   */
  function getExternalUrl() {
    $path = str_replace('\\', '/', DrupalStreamWrapperRegistry::getStreamTarget($this->uri));
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

  /**
   * Implements abstract public function getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('stream_private_path', 'sites/default/files-private');
  }

  /**
   * Overrides getExternalUrl().
   *
   * Return the HTML URI of a private file.
   */
  function getExternalUrl() {
    $path = str_replace('\\', '/', DrupalStreamWrapperRegistry::getStreamTarget($this->uri));
    return url('system/files/' . $path, array('absolute' => TRUE));
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

  /**
   * Implements abstract public function getDirectoryPath()
   */
  public function getDirectoryPath() {
    $temporary_directory = variable_get('stream_temp_path');

    if (is_null($temporary_directory)) {
      $directories = array();

      // Has PHP been set with an upload_tmp_dir?
      if (ini_get('upload_tmp_dir')) {
        $directories[] = ini_get('upload_tmp_dir');
      }

      // Operating system specific dirs.
      if (substr(PHP_OS, 0, 3) == 'WIN') {
        $directories[] = 'c:/windows/temp';
        $directories[] = 'c:/winnt/temp';
      }
      else {
        $directories[] = '/tmp';
      }

      foreach ($directories as $directory) {
        if (!$temporary_directory && is_dir($directory)) {
          $temporary_directory = $directory;
        }
      }

      // If a directory has been found, use it, otherwise default to 'files/tmp'
      $temporary_directory = $temporary_directory ? $temporary_directory : file_directory_path('public') . '/tmp';
      variable_set('stream_temp_path', $temporary_directory);
      return $temporary_directory;
    }

    return variable_get('stream_temp_path', '/tmp');
  }
}
