# git-format-patchset

## Overview

git-format-patchset is used for formatting patchsets for sending using `git send-email`

## Using git-format-patchset

Start by creating a `cover-letter` file for your project.
For example:

    $ base=origin/master; b=mybranch; v=v1; mkdir -p patches/$b/$v
    $ cat > patches/$b/$v/cover-letter
    Prefix: PATCH v1
    Cc: To whom <it-may-concern@my.org>
    Subject: Cleanup the code
    
    This series cleans up the code.
    
    Fixes #1234
    
    Test: mytest
    
    --
    Available also on git@github.com:myaccounnt/myrepo/mybranch.git mybranch-v1

The cover-letter header may contain the following items:
1. `Prefix: <prefix_text>`<br>
passed to `git format-patch --prefix <prefix_text>` to determine the text appearing in patches' email subject as `[PATCH v1 M/N]`
2. `(From|To|Cc): <email_addresses>[, <email_address[, ...]]`<br>
Optional, can be used to pass email addresses to `git send-email` for patch 0.
3. `Subject: <subject_text>`<br>
Used as the email subject for the cover-letter patch: PATCH 0.

The text following the header (skipping empty lines) is copied as a blurb into the the series `[PATCH v1 0/N]`

git-format-patchset uses `git format-patch` to produce the raw patches
and then it processes the first patch in the series - patch 0,
replacing the `*** SUBJECT HERE ***` marker with the cover-letter's `Subject`,
and the `*** BLURB HERE ***` marker with the rest of the cover-letter.

A typical command line for running git-format-patchset can be:

    $ git tag -f $b-$v $b
    $ git push --force myrepo $b $b-$v
    $ rm -f patches/$b/$v/*.patch
    $ git format-patchset patches/$b/$v $base..$b

Finally, to send the patchset to the mailing list:

    $ git send-email --to=scylladb-dev@googlegroups.com patches/$b/$v/*.patch
