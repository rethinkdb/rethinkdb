#!/usr/bin/env python
# Copyright 2013 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import datetime
import httplib
import glob
import imp
import json
import os
import re
import shutil
import subprocess
import sys
import textwrap
import time
import urllib
import urllib2

from git_recipes import GitRecipesMixin
from git_recipes import GitFailedException

VERSION_FILE = os.path.join("src", "version.cc")

# V8 base directory.
V8_BASE = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def TextToFile(text, file_name):
  with open(file_name, "w") as f:
    f.write(text)


def AppendToFile(text, file_name):
  with open(file_name, "a") as f:
    f.write(text)


def LinesInFile(file_name):
  with open(file_name) as f:
    for line in f:
      yield line


def FileToText(file_name):
  with open(file_name) as f:
    return f.read()


def MSub(rexp, replacement, text):
  return re.sub(rexp, replacement, text, flags=re.MULTILINE)


def Fill80(line):
  # Replace tabs and remove surrounding space.
  line = re.sub(r"\t", r"        ", line.strip())

  # Format with 8 characters indentation and line width 80.
  return textwrap.fill(line, width=80, initial_indent="        ",
                       subsequent_indent="        ")


def MakeComment(text):
  return MSub(r"^( ?)", "#", text)


def StripComments(text):
  # Use split not splitlines to keep terminal newlines.
  return "\n".join(filter(lambda x: not x.startswith("#"), text.split("\n")))


def MakeChangeLogBody(commit_messages, auto_format=False):
  result = ""
  added_titles = set()
  for (title, body, author) in commit_messages:
    # TODO(machenbach): Better check for reverts. A revert should remove the
    # original CL from the actual log entry.
    title = title.strip()
    if auto_format:
      # Only add commits that set the LOG flag correctly.
      log_exp = r"^[ \t]*LOG[ \t]*=[ \t]*(?:(?:Y(?:ES)?)|TRUE)"
      if not re.search(log_exp, body, flags=re.I | re.M):
        continue
      # Never include reverts.
      if title.startswith("Revert "):
        continue
      # Don't include duplicates.
      if title in added_titles:
        continue

    # Add and format the commit's title and bug reference. Move dot to the end.
    added_titles.add(title)
    raw_title = re.sub(r"(\.|\?|!)$", "", title)
    bug_reference = MakeChangeLogBugReference(body)
    space = " " if bug_reference else ""
    result += "%s\n" % Fill80("%s%s%s." % (raw_title, space, bug_reference))

    # Append the commit's author for reference if not in auto-format mode.
    if not auto_format:
      result += "%s\n" % Fill80("(%s)" % author.strip())

    result += "\n"
  return result


def MakeChangeLogBugReference(body):
  """Grep for "BUG=xxxx" lines in the commit message and convert them to
  "(issue xxxx)".
  """
  crbugs = []
  v8bugs = []

  def AddIssues(text):
    ref = re.match(r"^BUG[ \t]*=[ \t]*(.+)$", text.strip())
    if not ref:
      return
    for bug in ref.group(1).split(","):
      bug = bug.strip()
      match = re.match(r"^v8:(\d+)$", bug)
      if match: v8bugs.append(int(match.group(1)))
      else:
        match = re.match(r"^(?:chromium:)?(\d+)$", bug)
        if match: crbugs.append(int(match.group(1)))

  # Add issues to crbugs and v8bugs.
  map(AddIssues, body.splitlines())

  # Filter duplicates, sort, stringify.
  crbugs = map(str, sorted(set(crbugs)))
  v8bugs = map(str, sorted(set(v8bugs)))

  bug_groups = []
  def FormatIssues(prefix, bugs):
    if len(bugs) > 0:
      plural = "s" if len(bugs) > 1 else ""
      bug_groups.append("%sissue%s %s" % (prefix, plural, ", ".join(bugs)))

  FormatIssues("", v8bugs)
  FormatIssues("Chromium ", crbugs)

  if len(bug_groups) > 0:
    return "(%s)" % ", ".join(bug_groups)
  else:
    return ""


def SortingKey(version):
  """Key for sorting version number strings: '3.11' > '3.2.1.1'"""
  version_keys = map(int, version.split("."))
  # Fill up to full version numbers to normalize comparison.
  while len(version_keys) < 4:  # pragma: no cover
    version_keys.append(0)
  # Fill digits.
  return ".".join(map("{0:04d}".format, version_keys))


# Some commands don't like the pipe, e.g. calling vi from within the script or
# from subscripts like git cl upload.
def Command(cmd, args="", prefix="", pipe=True, cwd=None):
  cwd = cwd or os.getcwd()
  # TODO(machenbach): Use timeout.
  cmd_line = "%s %s %s" % (prefix, cmd, args)
  print "Command: %s" % cmd_line
  print "in %s" % cwd
  sys.stdout.flush()
  try:
    if pipe:
      return subprocess.check_output(cmd_line, shell=True, cwd=cwd)
    else:
      return subprocess.check_call(cmd_line, shell=True, cwd=cwd)
  except subprocess.CalledProcessError:
    return None
  finally:
    sys.stdout.flush()
    sys.stderr.flush()


# Wrapper for side effects.
class SideEffectHandler(object):  # pragma: no cover
  def Call(self, fun, *args, **kwargs):
    return fun(*args, **kwargs)

  def Command(self, cmd, args="", prefix="", pipe=True, cwd=None):
    return Command(cmd, args, prefix, pipe, cwd=cwd)

  def ReadLine(self):
    return sys.stdin.readline().strip()

  def ReadURL(self, url, params=None):
    # pylint: disable=E1121
    url_fh = urllib2.urlopen(url, params, 60)
    try:
      return url_fh.read()
    finally:
      url_fh.close()

  def ReadClusterFuzzAPI(self, api_key, **params):
    params["api_key"] = api_key.strip()
    params = urllib.urlencode(params)

    headers = {"Content-type": "application/x-www-form-urlencoded"}

    conn = httplib.HTTPSConnection("backend-dot-cluster-fuzz.appspot.com")
    conn.request("POST", "/_api/", params, headers)

    response = conn.getresponse()
    data = response.read()

    try:
      return json.loads(data)
    except:
      print data
      print "ERROR: Could not read response. Is your key valid?"
      raise

  def Sleep(self, seconds):
    time.sleep(seconds)

  def GetDate(self):
    return datetime.date.today().strftime("%Y-%m-%d")

  def GetUTCStamp(self):
    return time.mktime(datetime.datetime.utcnow().timetuple())

DEFAULT_SIDE_EFFECT_HANDLER = SideEffectHandler()


class NoRetryException(Exception):
  pass


class VCInterface(object):
  def InjectStep(self, step):
    self.step=step

  def Pull(self):
    raise NotImplementedError()

  def Fetch(self):
    raise NotImplementedError()

  def GetTags(self):
    raise NotImplementedError()

  def GetBranches(self):
    raise NotImplementedError()

  def GitSvn(self, hsh, branch=""):
    raise NotImplementedError()

  def SvnGit(self, rev, branch=""):
    raise NotImplementedError()

  def MasterBranch(self):
    raise NotImplementedError()

  def CandidateBranch(self):
    raise NotImplementedError()

  def RemoteMasterBranch(self):
    raise NotImplementedError()

  def RemoteCandidateBranch(self):
    raise NotImplementedError()

  def RemoteBranch(self, name):
    raise NotImplementedError()

  def Land(self):
    raise NotImplementedError()

  def CLLand(self):
    raise NotImplementedError()

  # TODO(machenbach): There is some svn knowledge in this interface. In svn,
  # tag and commit are different remote commands, while in git we would commit
  # and tag locally and then push/land in one unique step.
  def Tag(self, tag, remote, message):
    """Sets a tag for the current commit.

    Assumptions: The commit already landed and the commit message is unique.
    """
    raise NotImplementedError()


class GitSvnInterface(VCInterface):
  def Pull(self):
    self.step.GitSVNRebase()

  def Fetch(self):
    self.step.GitSVNFetch()

  def GetTags(self):
    # Get remote tags.
    tags = filter(lambda s: re.match(r"^svn/tags/[\d+\.]+$", s),
                  self.step.GitRemotes())

    # Remove 'svn/tags/' prefix.
    return map(lambda s: s[9:], tags)

  def GetBranches(self):
    # Get relevant remote branches, e.g. "svn/3.25".
    branches = filter(lambda s: re.match(r"^svn/\d+\.\d+$", s),
                      self.step.GitRemotes())
    # Remove 'svn/' prefix.
    return map(lambda s: s[4:], branches)

  def GitSvn(self, hsh, branch=""):
    return self.step.GitSVNFindSVNRev(hsh, branch)

  def SvnGit(self, rev, branch=""):
    return self.step.GitSVNFindGitHash(rev, branch)

  def MasterBranch(self):
    return "bleeding_edge"

  def CandidateBranch(self):
    return "trunk"

  def RemoteMasterBranch(self):
    return "svn/bleeding_edge"

  def RemoteCandidateBranch(self):
    return "svn/trunk"

  def RemoteBranch(self, name):
    return "svn/%s" % name

  def Land(self):
    self.step.GitSVNDCommit()

  def CLLand(self):
    self.step.GitDCommit()

  def Tag(self, tag, remote, _):
    self.step.GitSVNFetch()
    self.step.Git("rebase %s" % remote)
    self.step.GitSVNTag(tag)


class GitTagsOnlyMixin(VCInterface):
  def Pull(self):
    self.step.GitPull()

  def Fetch(self):
    self.step.Git("fetch")
    self.step.GitSVNFetch()

  def GetTags(self):
     return self.step.Git("tag").strip().splitlines()

  def GetBranches(self):
    # Get relevant remote branches, e.g. "branch-heads/3.25".
    branches = filter(
        lambda s: re.match(r"^branch\-heads/\d+\.\d+$", s),
        self.step.GitRemotes())
    # Remove 'branch-heads/' prefix.
    return map(lambda s: s[13:], branches)

  def MasterBranch(self):
    return "master"

  def CandidateBranch(self):
    return "candidates"

  def RemoteMasterBranch(self):
    return "origin/master"

  def RemoteCandidateBranch(self):
    return "origin/candidates"

  def RemoteBranch(self, name):
    if name in ["candidates", "master"]:
      return "origin/%s" % name
    return "branch-heads/%s" % name

  def PushRef(self, ref):
    self.step.Git("push origin %s" % ref)

  def Tag(self, tag, remote, message):
    # Wait for the commit to appear. Assumes unique commit message titles (this
    # is the case for all automated merge and push commits - also no title is
    # the prefix of another title).
    commit = None
    for wait_interval in [3, 7, 15, 35, 45, 60]:
      self.step.Git("fetch")
      commit = self.step.GitLog(n=1, format="%H", grep=message, branch=remote)
      if commit:
        break
      print("The commit has not replicated to git. Waiting for %s seconds." %
            wait_interval)
      self.step._side_effect_handler.Sleep(wait_interval)
    else:
      self.step.Die("Couldn't determine commit for setting the tag. Maybe the "
                    "git updater is lagging behind?")

    self.step.Git("tag %s %s" % (tag, commit))
    self.PushRef(tag)


class GitReadSvnWriteInterface(GitTagsOnlyMixin, GitSvnInterface):
  pass


class GitInterface(GitTagsOnlyMixin):
  def Fetch(self):
    self.step.Git("fetch")

  def GitSvn(self, hsh, branch=""):
    return ""

  def SvnGit(self, rev, branch=""):
    raise NotImplementedError()

  def Land(self):
    # FIXME(machenbach): This will not work with checkouts from bot_update
    # after flag day because it will push to the cache. Investigate if it
    # will work with "cl land".
    self.step.Git("push origin")

  def CLLand(self):
    self.step.GitCLLand()

  def PushRef(self, ref):
    self.step.Git("push https://chromium.googlesource.com/v8/v8 %s" % ref)


VC_INTERFACES = {
  "git_svn": GitSvnInterface,
  "git_read_svn_write": GitReadSvnWriteInterface,
  "git": GitInterface,
}


class Step(GitRecipesMixin):
  def __init__(self, text, number, config, state, options, handler):
    self._text = text
    self._number = number
    self._config = config
    self._state = state
    self._options = options
    self._side_effect_handler = handler
    self.vc = VC_INTERFACES[options.vc_interface]()
    self.vc.InjectStep(self)

    # The testing configuration might set a different default cwd.
    self.default_cwd = (self._config.get("DEFAULT_CWD") or
                        os.path.join(self._options.work_dir, "v8"))

    assert self._number >= 0
    assert self._config is not None
    assert self._state is not None
    assert self._side_effect_handler is not None

  def __getitem__(self, key):
    # Convenience method to allow direct [] access on step classes for
    # manipulating the backed state dict.
    return self._state[key]

  def __setitem__(self, key, value):
    # Convenience method to allow direct [] access on step classes for
    # manipulating the backed state dict.
    self._state[key] = value

  def Config(self, key):
    return self._config[key]

  def Run(self):
    # Restore state.
    state_file = "%s-state.json" % self._config["PERSISTFILE_BASENAME"]
    if not self._state and os.path.exists(state_file):
      self._state.update(json.loads(FileToText(state_file)))

    print ">>> Step %d: %s" % (self._number, self._text)
    try:
      return self.RunStep()
    finally:
      # Persist state.
      TextToFile(json.dumps(self._state), state_file)

  def RunStep(self):  # pragma: no cover
    raise NotImplementedError

  def Retry(self, cb, retry_on=None, wait_plan=None):
    """ Retry a function.
    Params:
      cb: The function to retry.
      retry_on: A callback that takes the result of the function and returns
                True if the function should be retried. A function throwing an
                exception is always retried.
      wait_plan: A list of waiting delays between retries in seconds. The
                 maximum number of retries is len(wait_plan).
    """
    retry_on = retry_on or (lambda x: False)
    wait_plan = list(wait_plan or [])
    wait_plan.reverse()
    while True:
      got_exception = False
      try:
        result = cb()
      except NoRetryException as e:
        raise e
      except Exception as e:
        got_exception = e
      if got_exception or retry_on(result):
        if not wait_plan:  # pragma: no cover
          raise Exception("Retried too often. Giving up. Reason: %s" %
                          str(got_exception))
        wait_time = wait_plan.pop()
        print "Waiting for %f seconds." % wait_time
        self._side_effect_handler.Sleep(wait_time)
        print "Retrying..."
      else:
        return result

  def ReadLine(self, default=None):
    # Don't prompt in forced mode.
    if self._options.force_readline_defaults and default is not None:
      print "%s (forced)" % default
      return default
    else:
      return self._side_effect_handler.ReadLine()

  def Command(self, name, args, cwd=None):
    cmd = lambda: self._side_effect_handler.Command(
        name, args, "", True, cwd=cwd or self.default_cwd)
    return self.Retry(cmd, None, [5])

  def Git(self, args="", prefix="", pipe=True, retry_on=None, cwd=None):
    cmd = lambda: self._side_effect_handler.Command(
        "git", args, prefix, pipe, cwd=cwd or self.default_cwd)
    result = self.Retry(cmd, retry_on, [5, 30])
    if result is None:
      raise GitFailedException("'git %s' failed." % args)
    return result

  def SVN(self, args="", prefix="", pipe=True, retry_on=None, cwd=None):
    cmd = lambda: self._side_effect_handler.Command(
        "svn", args, prefix, pipe, cwd=cwd or self.default_cwd)
    return self.Retry(cmd, retry_on, [5, 30])

  def Editor(self, args):
    if self._options.requires_editor:
      return self._side_effect_handler.Command(
          os.environ["EDITOR"],
          args,
          pipe=False,
          cwd=self.default_cwd)

  def ReadURL(self, url, params=None, retry_on=None, wait_plan=None):
    wait_plan = wait_plan or [3, 60, 600]
    cmd = lambda: self._side_effect_handler.ReadURL(url, params)
    return self.Retry(cmd, retry_on, wait_plan)

  def GetDate(self):
    return self._side_effect_handler.GetDate()

  def Die(self, msg=""):
    if msg != "":
      print "Error: %s" % msg
    print "Exiting"
    raise Exception(msg)

  def DieNoManualMode(self, msg=""):
    if not self._options.manual:  # pragma: no cover
      msg = msg or "Only available in manual mode."
      self.Die(msg)

  def Confirm(self, msg):
    print "%s [Y/n] " % msg,
    answer = self.ReadLine(default="Y")
    return answer == "" or answer == "Y" or answer == "y"

  def DeleteBranch(self, name):
    for line in self.GitBranch().splitlines():
      if re.match(r"\*?\s*%s$" % re.escape(name), line):
        msg = "Branch %s exists, do you want to delete it?" % name
        if self.Confirm(msg):
          self.GitDeleteBranch(name)
          print "Branch %s deleted." % name
        else:
          msg = "Can't continue. Please delete branch %s and try again." % name
          self.Die(msg)

  def InitialEnvironmentChecks(self, cwd):
    # Cancel if this is not a git checkout.
    if not os.path.exists(os.path.join(cwd, ".git")):  # pragma: no cover
      self.Die("This is not a git checkout, this script won't work for you.")

    # Cancel if EDITOR is unset or not executable.
    if (self._options.requires_editor and (not os.environ.get("EDITOR") or
        self.Command(
            "which", os.environ["EDITOR"]) is None)):  # pragma: no cover
      self.Die("Please set your EDITOR environment variable, you'll need it.")

  def CommonPrepare(self):
    # Check for a clean workdir.
    if not self.GitIsWorkdirClean():  # pragma: no cover
      self.Die("Workspace is not clean. Please commit or undo your changes.")

    # Persist current branch.
    self["current_branch"] = self.GitCurrentBranch()

    # Fetch unfetched revisions.
    self.vc.Fetch()

  def PrepareBranch(self):
    # Delete the branch that will be created later if it exists already.
    self.DeleteBranch(self._config["BRANCHNAME"])

  def CommonCleanup(self):
    if ' ' in self["current_branch"]:
      self.GitCheckout('master')
    else:
      self.GitCheckout(self["current_branch"])
    if self._config["BRANCHNAME"] != self["current_branch"]:
      self.GitDeleteBranch(self._config["BRANCHNAME"])

    # Clean up all temporary files.
    for f in glob.iglob("%s*" % self._config["PERSISTFILE_BASENAME"]):
      if os.path.isfile(f):
        os.remove(f)
      if os.path.isdir(f):
        shutil.rmtree(f)

  def ReadAndPersistVersion(self, prefix=""):
    def ReadAndPersist(var_name, def_name):
      match = re.match(r"^#define %s\s+(\d*)" % def_name, line)
      if match:
        value = match.group(1)
        self["%s%s" % (prefix, var_name)] = value
    for line in LinesInFile(os.path.join(self.default_cwd, VERSION_FILE)):
      for (var_name, def_name) in [("major", "MAJOR_VERSION"),
                                   ("minor", "MINOR_VERSION"),
                                   ("build", "BUILD_NUMBER"),
                                   ("patch", "PATCH_LEVEL")]:
        ReadAndPersist(var_name, def_name)

  def WaitForLGTM(self):
    print ("Please wait for an LGTM, then type \"LGTM<Return>\" to commit "
           "your change. (If you need to iterate on the patch or double check "
           "that it's sane, do so in another shell, but remember to not "
           "change the headline of the uploaded CL.")
    answer = ""
    while answer != "LGTM":
      print "> ",
      answer = self.ReadLine(None if self._options.wait_for_lgtm else "LGTM")
      if answer != "LGTM":
        print "That was not 'LGTM'."

  def WaitForResolvingConflicts(self, patch_file):
    print("Applying the patch \"%s\" failed. Either type \"ABORT<Return>\", "
          "or resolve the conflicts, stage *all* touched files with "
          "'git add', and type \"RESOLVED<Return>\"")
    self.DieNoManualMode()
    answer = ""
    while answer != "RESOLVED":
      if answer == "ABORT":
        self.Die("Applying the patch failed.")
      if answer != "":
        print "That was not 'RESOLVED' or 'ABORT'."
      print "> ",
      answer = self.ReadLine()

  # Takes a file containing the patch to apply as first argument.
  def ApplyPatch(self, patch_file, revert=False):
    try:
      self.GitApplyPatch(patch_file, revert)
    except GitFailedException:
      self.WaitForResolvingConflicts(patch_file)

  def FindLastTrunkPush(
      self, parent_hash="", branch="", include_patches=False):
    push_pattern = "^Version [[:digit:]]*\.[[:digit:]]*\.[[:digit:]]*"
    if not include_patches:
      # Non-patched versions only have three numbers followed by the "(based
      # on...) comment."
      push_pattern += " (based"
    branch = "" if parent_hash else branch or self.vc.RemoteCandidateBranch()
    return self.GitLog(n=1, format="%H", grep=push_pattern,
                       parent_hash=parent_hash, branch=branch)

  def ArrayToVersion(self, prefix):
    return ".".join([self[prefix + "major"],
                     self[prefix + "minor"],
                     self[prefix + "build"],
                     self[prefix + "patch"]])

  def SetVersion(self, version_file, prefix):
    output = ""
    for line in FileToText(version_file).splitlines():
      if line.startswith("#define MAJOR_VERSION"):
        line = re.sub("\d+$", self[prefix + "major"], line)
      elif line.startswith("#define MINOR_VERSION"):
        line = re.sub("\d+$", self[prefix + "minor"], line)
      elif line.startswith("#define BUILD_NUMBER"):
        line = re.sub("\d+$", self[prefix + "build"], line)
      elif line.startswith("#define PATCH_LEVEL"):
        line = re.sub("\d+$", self[prefix + "patch"], line)
      output += "%s\n" % line
    TextToFile(output, version_file)

  def SVNCommit(self, root, commit_message):
    patch = self.GitDiff("HEAD^", "HEAD")
    TextToFile(patch, self._config["PATCH_FILE"])
    self.Command("svn", "update", cwd=self._options.svn)
    if self.Command("svn", "status", cwd=self._options.svn) != "":
      self.Die("SVN checkout not clean.")
    if not self.Command("patch", "-d %s -p1 -i %s" %
                        (root, self._config["PATCH_FILE"]),
                        cwd=self._options.svn):
      self.Die("Could not apply patch.")
    for line in self.Command(
        "svn", "status", cwd=self._options.svn).splitlines():
      # Check for added and removed items. Svn status has seven status columns.
      # The first contains ? for unknown and ! for missing.
      match = re.match(r"^(.)...... (.*)$", line)
      if match and match.group(1) == "?":
        self.Command("svn", "add --force %s" % match.group(2),
                     cwd=self._options.svn)
      if match and match.group(1) == "!":
        self.Command("svn", "delete --force %s" % match.group(2),
                     cwd=self._options.svn)

    self.Command(
        "svn",
        "commit --non-interactive --username=%s --config-dir=%s -m \"%s\"" %
            (self._options.author, self._options.svn_config, commit_message),
        cwd=self._options.svn)


class BootstrapStep(Step):
  MESSAGE = "Bootstapping v8 checkout."

  def RunStep(self):
    if os.path.realpath(self.default_cwd) == os.path.realpath(V8_BASE):
      self.Die("Can't use v8 checkout with calling script as work checkout.")
    # Directory containing the working v8 checkout.
    if not os.path.exists(self._options.work_dir):
      os.makedirs(self._options.work_dir)
    if not os.path.exists(self.default_cwd):
      self.Command("fetch", "v8", cwd=self._options.work_dir)


class UploadStep(Step):
  MESSAGE = "Upload for code review."

  def RunStep(self):
    if self._options.reviewer:
      print "Using account %s for review." % self._options.reviewer
      reviewer = self._options.reviewer
    else:
      print "Please enter the email address of a V8 reviewer for your patch: ",
      self.DieNoManualMode("A reviewer must be specified in forced mode.")
      reviewer = self.ReadLine()
    self.GitUpload(reviewer, self._options.author, self._options.force_upload,
                   bypass_hooks=self._options.bypass_upload_hooks,
                   cc=self._options.cc)


class DetermineV8Sheriff(Step):
  MESSAGE = "Determine the V8 sheriff for code review."

  def RunStep(self):
    self["sheriff"] = None
    if not self._options.sheriff:  # pragma: no cover
      return

    try:
      # The googlers mapping maps @google.com accounts to @chromium.org
      # accounts.
      googlers = imp.load_source('googlers_mapping',
                                 self._options.googlers_mapping)
      googlers = googlers.list_to_dict(googlers.get_list())
    except:  # pragma: no cover
      print "Skip determining sheriff without googler mapping."
      return

    # The sheriff determined by the rotation on the waterfall has a
    # @google.com account.
    url = "https://chromium-build.appspot.com/p/chromium/sheriff_v8.js"
    match = re.match(r"document\.write\('(\w+)'\)", self.ReadURL(url))

    # If "channel is sheriff", we can't match an account.
    if match:
      g_name = match.group(1)
      self["sheriff"] = googlers.get(g_name + "@google.com",
                                     g_name + "@chromium.org")
      self._options.reviewer = self["sheriff"]
      print "Found active sheriff: %s" % self["sheriff"]
    else:
      print "No active sheriff found."


def MakeStep(step_class=Step, number=0, state=None, config=None,
             options=None, side_effect_handler=DEFAULT_SIDE_EFFECT_HANDLER):
    # Allow to pass in empty dictionaries.
    state = state if state is not None else {}
    config = config if config is not None else {}

    try:
      message = step_class.MESSAGE
    except AttributeError:
      message = step_class.__name__

    return step_class(message, number=number, config=config,
                      state=state, options=options,
                      handler=side_effect_handler)


class ScriptsBase(object):
  def __init__(self,
               config=None,
               side_effect_handler=DEFAULT_SIDE_EFFECT_HANDLER,
               state=None):
    self._config = config or self._Config()
    self._side_effect_handler = side_effect_handler
    self._state = state if state is not None else {}

  def _Description(self):
    return None

  def _PrepareOptions(self, parser):
    pass

  def _ProcessOptions(self, options):
    return True

  def _Steps(self):  # pragma: no cover
    raise Exception("Not implemented.")

  def _Config(self):
    return {}

  def MakeOptions(self, args=None):
    parser = argparse.ArgumentParser(description=self._Description())
    parser.add_argument("-a", "--author", default="",
                        help="The author email used for rietveld.")
    parser.add_argument("--dry-run", default=False, action="store_true",
                        help="Perform only read-only actions.")
    parser.add_argument("-g", "--googlers-mapping",
                        help="Path to the script mapping google accounts.")
    parser.add_argument("-r", "--reviewer", default="",
                        help="The account name to be used for reviews.")
    parser.add_argument("--sheriff", default=False, action="store_true",
                        help=("Determine current sheriff to review CLs. On "
                              "success, this will overwrite the reviewer "
                              "option."))
    parser.add_argument("--svn",
                        help=("Optional full svn checkout for the commit."
                              "The folder needs to be the svn root."))
    parser.add_argument("--svn-config",
                        help=("Optional folder used as svn --config-dir."))
    parser.add_argument("-s", "--step",
        help="Specify the step where to start work. Default: 0.",
        default=0, type=int)
    parser.add_argument("--vc-interface",
                        help=("Choose VC interface out of git_svn|"
                              "git_read_svn_write."))
    parser.add_argument("--work-dir",
                        help=("Location where to bootstrap a working v8 "
                              "checkout."))
    self._PrepareOptions(parser)

    if args is None:  # pragma: no cover
      options = parser.parse_args()
    else:
      options = parser.parse_args(args)

    # Process common options.
    if options.step < 0:  # pragma: no cover
      print "Bad step number %d" % options.step
      parser.print_help()
      return None
    if options.sheriff and not options.googlers_mapping:  # pragma: no cover
      print "To determine the current sheriff, requires the googler mapping"
      parser.print_help()
      return None
    if options.svn and not options.svn_config:
      print "Using pure svn for committing requires also --svn-config"
      parser.print_help()
      return None

    # Defaults for options, common to all scripts.
    options.manual = getattr(options, "manual", True)
    options.force = getattr(options, "force", False)
    options.bypass_upload_hooks = False

    # Derived options.
    options.requires_editor = not options.force
    options.wait_for_lgtm = not options.force
    options.force_readline_defaults = not options.manual
    options.force_upload = not options.manual

    # Process script specific options.
    if not self._ProcessOptions(options):
      parser.print_help()
      return None

    if not options.vc_interface:
      options.vc_interface = "git_read_svn_write"
    if not options.work_dir:
      options.work_dir = "/tmp/v8-release-scripts-work-dir"
    return options

  def RunSteps(self, step_classes, args=None):
    options = self.MakeOptions(args)
    if not options:
      return 1

    state_file = "%s-state.json" % self._config["PERSISTFILE_BASENAME"]
    if options.step == 0 and os.path.exists(state_file):
      os.remove(state_file)

    steps = []
    for (number, step_class) in enumerate([BootstrapStep] + step_classes):
      steps.append(MakeStep(step_class, number, self._state, self._config,
                            options, self._side_effect_handler))
    for step in steps[options.step:]:
      if step.Run():
        return 0
    return 0

  def Run(self, args=None):
    return self.RunSteps(self._Steps(), args)
