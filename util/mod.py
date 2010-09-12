#!/usr/local/bin/python
import os
import os.path
import re
from collections import defaultdict

RESOURCE_DIR = "../resources"

instances = defaultdict(lambda: defaultdict(list))

for f in os.listdir(RESOURCE_DIR):
    if not re.search(r"\.(js|xml)$", f):
        continue
    for line in open(os.path.join(RESOURCE_DIR, f)).readlines():
        matches = re.match(r".*@(.*? .*?), (.*?), (.*)$", line)
        if matches:
            type, pos, name = matches.groups()
            instances[type][int(pos)].append(name)

for type in sorted(instances.keys()):
    print type + ":"
    for pos in sorted(instances[type].keys()):
        print "  %s: " % pos, ", ".join(instances[type][pos]) 
        
            

    