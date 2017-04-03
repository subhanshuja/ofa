import os
import errno
import sys
import shutil
import subprocess
import stat
import zipfile

class PathNotFound(Exception): pass
class ExternalError(Exception): pass

def IsWindows():
  return sys.platform == 'cygwin' or sys.platform.startswith('win')

def IsLinux():
  return sys.platform.startswith('linux')

def IsMac():
  return sys.platform.startswith('darwin')

def MakeWorldReadable(path):
  """Change the permissions of the given path to make it world-readable.
  This is often needed for archived files, so they can be served by web servers
  or accessed by unprivileged network users."""

  # No need to do anything special on Windows.
  if IsWindows():
    return

  perms = stat.S_IMODE(os.stat(path)[stat.ST_MODE])
  if os.path.isdir(path):
    # Directories need read and exec.
    os.chmod(path, perms | 0555)
  else:
    os.chmod(path, perms | 0444)

def MaybeMakeDirectory(*path):
  """Creates an entire path, if it doesn't already exist."""
  file_path = os.path.join(*path)
  try:
    os.makedirs(file_path)
  except OSError, e:
    if e.errno != errno.EEXIST:
      raise

def RemoveFile(*path):
  """Removes the file located at 'path', if it exists."""
  file_path = os.path.join(*path)
  try:
    os.remove(file_path)
  except OSError, e:
    if e.errno != errno.ENOENT:
      raise

def MoveFile(path, new_path):
  """Moves the file located at 'path' to 'new_path', if it exists."""
  try:
    RemoveFile(new_path)
    os.rename(path, new_path)
  except OSError, e:
    if e.errno != errno.ENOENT:
      raise

def RemoveDirectory(*path):
  """Recursively removes a directory, even if it's marked read-only.

  Remove the directory located at *path, if it exists.

  shutil.rmtree() doesn't work on Windows if any of the files or directories
  are read-only, which svn repositories and some .svn files are.  We need to
  be able to force the files to be writable (i.e., deletable) as we traverse
  the tree.

  Even with all this, Windows still sometimes fails to delete a file, citing
  a permission error (maybe something to do with antivirus scans or disk
  indexing).  The best suggestion any of the user forums had was to wait a
  bit and try again, so we do that too.  It's hand-waving, but sometimes it
  works. :/
  """
  file_path = os.path.join(*path)
  if not os.path.exists(file_path):
    return

  if sys.platform == 'win32':
    # Give up and use cmd.exe's rd command.
    file_path = os.path.normcase(file_path)
    for _ in xrange(3):
      if not subprocess.call(['cmd.exe', '/c', 'rd', '/q', '/s', file_path]):
        break
      time.sleep(3)
    return

  def RemoveWithRetry_non_win(rmfunc, path):
    if os.path.islink(path):
      return os.remove(path)
    else:
      return rmfunc(path)

  remove_with_retry = RemoveWithRetry_non_win

  def RmTreeOnError(function, path, excinfo):
    """This works around a problem whereby python 2.x on Windows has no ability
    to check for symbolic links.  os.path.islink always returns False.  But
    shutil.rmtree will fail if invoked on a symbolic link whose target was
    deleted before the link.  E.g., reproduce like this:
    > mkdir test
    > mkdir test\1
    > mklink /D test\current test\1
    > python -c "import chromium_utils; chromium_utils.RemoveDirectory('test')"
    To avoid this issue, we pass this error-handling function to rmtree.  If
    we see the exact sort of failure, we ignore it.  All other failures we re-
    raise.
    """

    exception_type = excinfo[0]
    exception_value = excinfo[1]
    # If shutil.rmtree encounters a symbolic link on Windows, os.listdir will
    # fail with a WindowsError exception with an ENOENT errno (i.e., file not
    # found).  We'll ignore that error.  Note that WindowsError is not defined
    # for non-Windows platforms, so we use OSError (of which it is a subclass)
    # to avoid lint complaints about an undefined global on non-Windows
    # platforms.
    if (function is os.listdir) and issubclass(exception_type, OSError):
      if exception_value.errno == errno.ENOENT:
        # File does not exist, and we're trying to delete, so we can ignore the
        # failure.
        print 'WARNING:  Failed to list %s during rmtree.  Ignoring.\n' % path
      else:
        raise
    else:
      raise

  for root, dirs, files in os.walk(file_path, topdown=False):
    # For POSIX:  making the directory writable guarantees removability.
    # Windows will ignore the non-read-only bits in the chmod value.
    os.chmod(root, 0770)
    for name in files:
      remove_with_retry(os.remove, os.path.join(root, name))
    for name in dirs:
      remove_with_retry(lambda p: shutil.rmtree(p, onerror=RmTreeOnError),
                        os.path.join(root, name))

  remove_with_retry(os.rmdir, file_path)

def CopyFileToDir(src_path, dest_dir, dest_fn=None):
  """Copies the file found at src_path to the dest_dir directory.

  If dest_fn is specified, the src_path is copied to that name in dest_dir,
  otherwise it is copied to a file of the same name.

  Raises PathNotFound if either the file or the directory is not found.
  """
  # Verify the file and directory separately so we can tell them apart and
  # raise PathNotFound rather than shutil.copyfile's IOError.
  if not os.path.isfile(src_path):
    raise PathNotFound('Unable to find file %s' % src_path)
  if not os.path.isdir(dest_dir):
    raise PathNotFound('Unable to find dir %s' % dest_dir)
  src_file = os.path.basename(src_path)
  if dest_fn:
    shutil.copy(src_path, os.path.join(dest_dir, dest_fn))
  else:
    shutil.copy(src_path, os.path.join(dest_dir, src_file))


def MakeZip(output_dir, archive_name, file_list, file_relative_dir,
            raise_error=True, remove_archive_directory=True, path_filter=None):
  """Packs files into a new zip archive.

  Files are first copied into a directory within the output_dir named for
  the archive_name, which will be created if necessary and emptied if it
  already exists.  The files are then then packed using archive names
  relative to the output_dir.  That is, if the zipfile is unpacked in place,
  it will create a directory identical to the new archiev_name directory, in
  the output_dir.  The zip file will be named as the archive_name, plus
  '.zip'.

  Args:
    output_dir: Absolute path to the directory in which the archive is to
      be created.
    archive_dir: Subdirectory of output_dir holding files to be added to
      the new zipfile.
    file_list: List of paths to files or subdirectories, relative to the
      file_relative_dir.
    file_relative_dir: Absolute path to the directory containing the files
      and subdirectories in the file_list.
    raise_error: Whether to raise a PathNotFound error if one of the files in
      the list is not found.
    remove_archive_directory: Whether to remove the archive staging directory
      before copying files over to it.
    path_filter: A function f(path) -> path. For each file to add, transform
      the path with the path_filter function, and read data from the resulting
      file instead. i.e. foo.exe could actually contain data from foo.asan.exe.

  Returns:
    A tuple consisting of (archive_dir, zip_file_path), where archive_dir
    is the full path to the newly created archive_name subdirectory.

  Raises:
    PathNotFound if any of the files in the list is not found, unless
    raise_error is False, in which case the error will be ignored.
  """

  # Collect files into the archive directory.
  archive_dir = os.path.join(output_dir, archive_name)
  if remove_archive_directory and os.path.exists(archive_dir):
    # Move it even if it's not a directory as expected. This can happen with
    # FILES.cfg archive creation where we create an archive staging directory
    # that is the same name as the ultimate archive name.
    if not os.path.isdir(archive_dir):
      print 'Moving old "%s" file to create same name directory.' % archive_dir
      previous_archive_file = '%s.old' % archive_dir
      MoveFile(archive_dir, previous_archive_file)
    else:
      RemoveDirectory(archive_dir)
  MaybeMakeDirectory(archive_dir)
  for needed_file in file_list:
    needed_file = needed_file.rstrip()
    # These paths are relative to the file_relative_dir.  We need to copy
    # them over maintaining the relative directories, where applicable.
    src_path = os.path.join(file_relative_dir, needed_file)
    if path_filter:
      new_src_path = path_filter(src_path)
      if new_src_path is None:
        print 'Skipping %s' % src_path
        continue
      if new_src_path != src_path:
        print 'Replacing contents of %s with %s' % (src_path, new_src_path)
      src_path = new_src_path
    dirname, basename = os.path.split(needed_file)
    try:
      if os.path.isdir(src_path):
        shutil.copytree(src_path, os.path.join(archive_dir, needed_file),
                        symlinks=True)
      elif dirname != '' and basename != '':
        dest_dir = os.path.join(archive_dir, dirname)
        MaybeMakeDirectory(dest_dir)
        CopyFileToDir(src_path, dest_dir, basename)
      else:
        CopyFileToDir(src_path, archive_dir, basename)
    except PathNotFound:
      if raise_error:
        raise

  # Pack the zip file.
  output_file = '%s.zip' % archive_dir
  previous_file = '%s_old.zip' % archive_dir
  MoveFile(output_file, previous_file)
  # On Windows we use the python zip module; on Linux and Mac, we use the zip
  # command as it will handle links and file bits (executable).  Which is much
  # easier then trying to do that with ZipInfo options.
  if IsWindows():
    print 'Creating %s' % output_file

    def _Addfiles(to_zip_file, dirname, files_to_add):
      for this_file in files_to_add:
        archive_name = this_file
        this_path = os.path.join(dirname, this_file)
        if os.path.isfile(this_path):
          # Store files named relative to the outer output_dir.
          archive_name = this_path.replace(output_dir + os.sep, '')
          if os.path.getsize(this_path) == 0:
            compress_method = zipfile.ZIP_STORED
          else:
            compress_method = zipfile.ZIP_DEFLATED
          to_zip_file.write(this_path, archive_name, compress_method)
          print 'Adding %s' % archive_name
    zip_file = zipfile.ZipFile(output_file, 'w', zipfile.ZIP_DEFLATED,
                               allowZip64=True)
    try:
      os.path.walk(archive_dir, _Addfiles, zip_file)
    finally:
      zip_file.close()
  else:
    assert IsMac() or IsLinux()
    saved_dir = os.getcwd()
    os.chdir(os.path.dirname(archive_dir))
    command = ['zip', '-yr1', output_file, os.path.basename(archive_dir)]
    #    result = RunCommand(command)
    result = subprocess.check_call(command, cwd=os.path.dirname(archive_dir))
    os.chdir(saved_dir)
    if result and raise_error:
      raise ExternalError('zip failed: %s => %s' %
                          (str(command), result))
  return (archive_dir, output_file)
