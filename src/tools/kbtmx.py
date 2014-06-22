import sys
import xml.etree.ElementTree as ET
import numpy as np
import re
import struct

# Note: this script is know not to work in Python 3 (for unknown reason)
# So please run it using Python 2.7 or similar, until the issue is resolved

if len(sys.argv) < 3:
	print "Usage: python kbtmx.py SOURCE DESTDIR"
	exit(0)

tmxfile = sys.argv[1]
outdir = sys.argv[2]

tree = ET.parse(tmxfile)
root = tree.getroot()

foes = [ ]
castles = [ ]
towns = [ ]
hotspots = [ ]

map = np.zeros([4, 64, 64], int)

cont = 0

for layer in root.findall("layer"):
	#print layer.tag, layer.attrib

	name = layer.get("name") 

	m = re.match(r"Continent (\d+)", name)
	if m is not None:
		cont = int(m.group(1))
		print "Reading landscape for continent %d" % cont 
	else:
		print "Undefined layer %s" % name
		continue	

	x = 0
	y = 0
	for tile in layer.find("data"):
		#print tile.attrib, tile.get("gid")

		#print "Setting %d %d %d " % (cont, y, x)
		
		tile = int(tile.get("gid")) - 1
		
		if tile < 0 or tile > 72:
			print "Warning: Tile %d/%dx%d is out of range!" % (cont, x, y)
			tile = 0

		map[cont][y][x] = tile 

		x = x + 1
		if x >= 64:
			x = 0
			y = y + 1

# Place salt marks
map[0][63][0] = 0xff
map[1][63][0] = 0xff
map[2][63][0] = 0xff
map[3][63][0] = 0xff

for objgroup in root.findall("objectgroup"):

	name = objgroup.get("name")
	
	print "Reading %s " % name

	for object in objgroup:
		print object.tag, object.attrib
		
		
# Save!

mapfile = outdir + "/" + "land.org"

print map

print "Saving to %s" % mapfile

fh = open(mapfile, "wb")
for cont in xrange(0, 4):
	for y in xrange(0, 64):
		for x in xrange(0, 64):
			fh.write(struct.pack("B", map[cont][63-y][x]))
fh.close()
