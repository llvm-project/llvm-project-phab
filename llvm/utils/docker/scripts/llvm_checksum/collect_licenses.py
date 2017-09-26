"""A script to copy all licences of LLVM checkout.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from argparse import ArgumentParser
from project_tree import *
import shutil
import os


def main():
  parser = ArgumentParser()
  parser.add_argument(
      "-v", "--verbose", action="store_true", help="enable debug logging")
  parser.add_argument(
      "--multi_dir",
      action="store_true",
      help="indicates llvm_path contains llvm, checked out " +
      "into multiple directories, as opposed to a " +
      "typical single source tree checkout")
  parser.add_argument(
      "--target", required=True, help="path to store the licences")
  parser.add_argument(
      "--llvm_path", required=True, help="path to LLVM checkout")

  args = parser.parse_args()

  if args.verbose:
    logging.basicConfig(level=logging.DEBUG)

  llvm_projects = CreateLLVMProjects(not args.multi_dir)
  CopyLicences(args.llvm_path, llvm_projects, args.target)


def CopyLicences(root_path, projects, target_path):
  for proj in projects:
    project_root = os.path.join(root_path, proj.relpath)
    if not os.path.exists(project_root):
      logging.info("Folder %s doesn't exist, skipping project %s", proj.relpath,
                   proj.name)
      continue

    def copy_license_file(file_path):
      logging.info("Copying licence file %s", file_path)
      relpath = os.path.relpath(file_path, project_root)
      target_licence_path = os.path.join(target_path, proj.name, relpath)

      target_dir = os.path.dirname(target_licence_path)
      if not os.path.exists(target_dir):
        os.makedirs(target_dir)
      shutil.copyfile(file_path, target_licence_path)

    def proccess_file_for_licences(file_path):
      file_name = os.path.basename(file_path)
      if file_name.lower() != "license.txt":
        return
      copy_license_file(file_path)

    # Copy all LICENCSE.txt files.
    WalkProjectFiles(root_path, projects, proj, proccess_file_for_licences)
    # Copy various extra licences for known projects. This list may be
    # incomplete or out of date.
    if proj.name == "llvm":
      copy_license_file(os.path.join(project_root, "lib", "Support", "MD5.cpp"))
      copy_license_file(
          os.path.join(project_root, "include", "llvm", "Support", "MD5.h"))
      copy_license_file(
          os.path.join(project_root, "lib", "Support", "COPYRIGHT.regex"))


if __name__ == "__main__":
  main()
