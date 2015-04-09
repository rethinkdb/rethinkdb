This repository contains hashes of build tools used by Chromium and related
projects. The actual binaries are pulled from Google Storage, normally as part
of a gclient hook.

The repository is separate so that the shared build tools can be shared between
the various Chromium-related projects without each one needing to maintain
their own versionining of each binary.
