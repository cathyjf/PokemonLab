"""Check if moves of pokemon in species.xml match moves in moves.xml

Useful for finding capitalization errors/misspellings
"""

from xml.etree import ElementTree

mt=ElementTree.parse(open('moves.xml'))
movenames = set(m.attrib['name'] for m in mt.findall('move'))

t=ElementTree.parse(open('species.xml'))
parent_map = dict((c, p) for p in t.getiterator() for c in p)

for m in [m for m in t.findall('*/*/*/move') if m.text not in movenames]:
    print parent_map[parent_map[parent_map[m]]].attrib['name'] + ':', m.text
