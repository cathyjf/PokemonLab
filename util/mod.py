#!/usr/local/bin/python
import os
import os.path
import re
import itertools

RESOURCE_DIR = "../resources"

instances = []

for f in os.listdir(RESOURCE_DIR):
    if not re.search(r"\.(js|xml)$", f):
        continue
    for line in open(os.path.join(RESOURCE_DIR, f)).readlines():
        matches = re.match(r".*@(.*?) (.*?), (.*?), (.*)$", line)
        if matches:
            instances.append(matches.groups())
            
for key, group in itertools.groupby(sorted(instances), key=lambda x: "%s %s" % (x[0], x[1])):
    print "%s:" % key
    for order, effects in itertools.groupby(sorted(group, key=lambda x: int(x[2])), key=lambda x: x[2]):
        print "  %s:" % order, ", ".join([x[-1] for x in list(effects)])
    print
    