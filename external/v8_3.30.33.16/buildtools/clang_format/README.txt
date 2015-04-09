This folder contains clang-format binaries. The binaries will be automatically
downloaded from Google Storage by gclient runhooks for the current platform.

For a walkthrough on how to maintain these binaries:
  https://code.google.com/p/chromium/wiki/UpdatingClangFormatBinaries

To upload a file:
  python ~/depot_tools/upload_to_google_storage.py -b chromium-clang-format <FILENAME>

To download a file given a .sha1 file:
  python ~/depot_tools/download_from_google_storage.py -b chromium-clang-format -s <FILENAME>.sha1

List the contents of GN's Google Storage bucket:
  python ~/depot_tools/third_party/gsutil/gsutil ls gs://chromium-clang-format/

To initialize gsutil's credentials:
  python ~/depot_tools/third_party/gsutil/gsutil config

  That will give a URL which you should log into with your web browser. The
  username should be the one that is on the ACL for the "chromium-clang-format"
  bucket (probably your @google.com address). Contact the build team for help
  getting access if necessary.

  Copy the code back to the command line util. Ignore the project ID (it's OK
  to just leave blank when prompted).

gsutil documentation:
  https://developers.google.com/storage/docs/gsutil
