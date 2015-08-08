#!/usr/bin/env python2.7
'''
Copyright (c) 2008-2014, Pedigree Developers

Please see the CONTRIB file in the root of the source tree for a full
list of contributors.

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'''

# This script is designed to configure a pup config file with the paths from
# this checkout of Pedigree.

import os
import sys

scriptdir = os.path.dirname(os.path.realpath(__file__))

pupConfigDefault = "%s/scripts/pup/pup.conf.default" % (scriptdir,)
pupConfig = "%s/scripts/pup/pup.conf" % (scriptdir,)

target_arch = 'i686'
if len(sys.argv) > 1:
    target_arch = sys.argv[1]

# Open the original file
with open(pupConfigDefault, "r") as default:
    with open(pupConfig, "w") as new:
        s = default.read()
        s = s.replace("$installRoot", "%s/images/local" % (scriptdir,))
        s = s.replace("$localDb", "%s/images/local/support/pup/db" % (scriptdir,))
        s += '\n\n[settings]\narch=%s' % (target_arch,)
        new.write(s)

print("Configuration file '%s' updated." % (pupConfig))

