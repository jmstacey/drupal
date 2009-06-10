<?php
// $Id: dummy_stream_wrapper.inc,v 1.1 2008/05/05 18:23:31 foo Exp $

/**
 * @file
 * Dummy stream wrapper used for testing stream wrapper support.
 */

/**
 * Dummy stream wrapper used for testing stream wrapper support.
 *
 * Files are stored with the path found by stripping the wrapper prefix. E.g.
 * "dummy-wrapper:///tmp/foo" maps to "/tmp/foo". Nested wrappers are also
 * supported, e.g. "dummy-wrapper://mywrapper://foo.txt" maps to
 * "mywrapper://foo.txt".
 *
 * The class implements the methods required by PHP's stream_wrapper_register().
 */
class DummyStreamWrapper {
  /**
   * A file handle to the nested file opened by stream_open().
   */
  private $fileHandle;

  /**
   * A directory handle to the nested file opened by dir_opendir().
   */
  private $directoryHandle;

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
   * @return
   *  TRUE if file was opened successfully.
   */
  public function stream_open($path, $mode, $options, &$opened_path) {
     $nestedPath = self::getNestedPath($path);
    if ($options & STREAM_REPORT_ERRORS) {
      $this->fileHandle = fopen($nestedPath, $mode);
    }
    else {
      $this->fileHandle = @fopen($nestedPath, $mode);
    }
    return (bool)$this->fileHandle;
  }

  /**
   * Support for fread(), file_get_contents() etc.
   *
   * @param $count
   *    Maximum number of bytes to be read.
   * @return
   *  The string that was read, or FALSE in case of an error.
   */
  public function stream_read($count) {
    return fread($this->fileHandle, $count);
  }

  /**
   * Support for fwrite(), file_put_contents() etc.
   *
   * @param $data
   *   The string to be written.
   * @return
   *   The number of bytes written.
   */
  public function stream_write($data) {
    return fwrite($this->fileHandle, $data);
  }

  /**
   * Support for feof().
   *
   * @return
   *   TRUE if end-of-file has been reached.
   */
  public function stream_eof() {
    return feof($this->fileHandle);
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
   */
  public function stream_seek($offset, $whence) {
    return fseek($this->fileHandle, $offset, $whence);
  }

  /**
   * Support for fflush().
   *
   * @return
   *   TRUE if data was successfully stored (or there was no data to store).
   */
  public function stream_flush() {
    return fflush($this->fileHandle);
  }

  /**
   * Support for ftell().
   *
   * @return
   *   The current offset in bytes from the beginning of file.
   */
  public function stream_tell() {
    return ftell($this->fileHandle);
  }

  /**
   * Support for fstat().
   *
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   */
  public function stream_stat() {
    return fstat($this->fileHandle);
  }

  /**
   * Support for fclose().
   *
   * @return
   *   TRUE if stream was successfully closed.
   */
  public function stream_close() {
    return fclose($this->fileHandle);
  }

  /**
   * Support for unlink().
   *
   * @param $path
   *   A string containing the path to the file to delete.
   * @return
   *   TRUE if file was successfully deleted.
   */
  public function unlink($path) {
    return unlink(self::getNestedPath($path));
  }

  /**
   * Support for rename().
   *
   * @param $fromPath
   *   The path to the file to rename.
   * @param $toPath
   *   The new path to the file.
   *
   * @return
   *   TRUE if file was successfully renamed.
   */
  public function rename($fromPath, $toPath) {
    return rename(self::getNestedPath($fromPath), self::getNestedPath($toPath));
  }

  /**
   * Support for mkdir().
   *
   * @param $path
   *   A string containing the path to the directory to create.
   * @param $mode
   *   Permission flags - see mkdir().
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS and STREAM_MKDIR_RECURSIVE.
   * @return
   *   TRUE if directory was successfully created.
   */
  public function mkdir($path, $mode, $options) {
    $nestedPath = self::getNestedPath($path);
    $recursive = (bool)($options & STREAM_MKDIR_RECURSIVE);
    if ($options & STREAM_REPORT_ERRORS) {
      return mkdir($nestedPath, $mode, $recursive);
    }
    else {
      return @mkdir($nestedPath, $mode, $recursive);
    }
  }

  /**
   * Support for rmdir().
   *
   * @param $path
   *   A string containing the path to the directory to delete.
   * @param $options
   *   A bit mask of STREAM_REPORT_ERRORS.
   * @return
   *   TRUE if directory was successfully removed.
   */
  public function rmdir($path, $options) {
    $nestedPath = self::getNestedPath($path);
    if ($options & STREAM_REPORT_ERRORS) {
      return rmdir($nestedPath);
    }
    else {
      return @rmdir($nestedPath);
    }
  }

  /**
   * Support for stat().
   *
   * @param $path
   *   A string containing the path to get information about.
   * @param $flags
   *   A bit mask of STREAM_URL_STAT_LINK and STREAM_URL_STAT_QUIET.
   * @return
   *   An array with file status, or FALSE in case of an error - see fstat()
   *   for a description of this array.
   */
  public function url_stat($path, $flags) {
    $nestedPath = self::getNestedPath($path);
    return ($flags & STREAM_URL_STAT_QUIET) ? (file_exists($nestedPath) ? stat($nestedPath) : FALSE) : stat($nestedPath);
  }

  /**
   * Support for opendir().
   *
   * @param $path
   *   A string containing the path to the directory to open.
   * @param $options
   *   Whether or not to enforce safe_mode.
   * @return
   *   TRUE on success.
   */
  public function dir_opendir($path, $options) {
    $this->dirHandle = opendir(self::getNestedPath($path));
    return (bool)$this->dirHandle;
  }

  /**
   * Support for readdir().
   *
   * @return
   *   The next filename, or FALSE if there are no more files in the directory.
   */
  public function dir_readdir() {
    return readdir($this->dirHandle);
  }

  /**
   * Support for rewinddir().
   *
   * @return
   *   TRUE on success.
   */
  public function dir_rewinddir() {
    return rewinddir($this->dirHandle);
  }

  /**
   * Support for closedir().
   *
   * @return
   *   TRUE on success.
   */
  public function dir_closedir() {
    return closedir($this->dirHandle);
  }

  /**
   * Strip the stream wrapper prefix from the specified path.
   *
   * @param $path
   *    A string containing a path with a stream wrapper prefix.
   * @return
   *    The without a stream wrapper prefix.
   */
  private static function getNestedPath($path) {
    return preg_replace('@^([a-z0-9.+-]{2,})://@i', '', $path);
  }
}

