import socket
sock = socket.socket( socket.AF_INET, # Internet
                      socket.SOCK_DGRAM ) # UDP

import time
import random

d = {}


def display():
  m = "".join(["".join([chr(d.get((x,y), (0,0,0))[0])+chr(d.get((x,y), (0,0,0))[1])+chr(d.get((x,y), (0,0,0))[2]) for x in range(25) ]) for y in range(12)])
  l = sock.sendto('\x01' + m, ("192.168.1.99", 58082) )


def readImgFile(filepath):
    f = open(filepath, "rb")

    i = Image()

    # Default assumption
    i.width = 25
    i.height = 12

    try:
      i.width, i.height = [int(j) for j in filepath.split("-")[1].split("x")]
    except IndexError:
      # Use default instead
      pass

    d = {}

    for y in range(0, i.height):
        for x in range(0, i.width):
            r,g,b = f.read(3)

            r = ord(r)
            g = ord(g)
            b = ord(b)
        
            d[(x,y)] = (r,g,b)

    i.data = d

    f.close()

    return i


class Image:

  width = 0

  height = 0

  data = None

  def __init__(self, imgToCopy = None, metaDataCopyOnly = False):
    
    self.data = {}
    
    if imgToCopy:
      self.width = imgToCopy.width
      self.height = imgToCopy.height

      if not metaDataCopyOnly:
        self.data = imgToCopy.data.copy()


def Filter_FakeAlpha(img):

    CUT_OFF = 0x40

    new_img = Image(img, True)
  
    for k,v in img.data.iteritems():
      if not (v[0] < CUT_OFF and v[1] < CUT_OFF and v[2] < CUT_OFF):
        new_img.data[k] = v

    return new_img


def Filter_Offset(img, (xOffset, yOffset)):

  new_img = Image(img, True)

  for (x, y) ,v in img.data.iteritems():
    new_img.data[(x - xOffset, y - yOffset)] = v

  return new_img


def ImgFile(filepath):
  return readImgFile(filepath) 


def Factory_Image(img):
  while True:
    yield img.data


def EffectFactory_Pulse(img):
  while True:
    for brightness in [0.0, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
                       1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, 0.05, 0.0]:

      new_img = Image(img, True)

      for k,v in img.data.iteritems():
        new_img.data[k] = tuple([int(i * brightness) for i in v])

      yield new_img.data.copy()


def keepRegion(data, (lx, ly), (rx, ry)):
  new_data = {}

  for (x,y), v in data.iteritems():
    if ((lx <= x < rx) and (ly <= y < ry)):
      new_data[(x,y)] = v

  return new_data
  

BLANK = ((0,0), (0,0))
FULL = ((0,0), (25, 12))
LEFT = ((0,0), (8,12))
MIDDLE = ((8, 0), (17, 12))
RIGHT = ((17, 0), (25, 12))
def EffectFactory_PanelCycle(img):
  while True:
    for l, r in [
                 LEFT, LEFT,
                 MIDDLE, MIDDLE,
                 RIGHT, RIGHT,
                 BLANK, BLANK,
                 BLANK, BLANK,
                 # RIGHT, RIGHT,
                 # MIDDLE, MIDDLE,
                 # LEFT, LEFT,
                 # BLANK, BLANK,
                 # BLANK, BLANK,
                 FULL, FULL,
                 FULL, FULL,
                 BLANK, BLANK,
                 BLANK, BLANK,
                 FULL, FULL,
                 FULL, FULL,
                 BLANK, BLANK,
                 BLANK, BLANK,
                 ]:

      yield keepRegion(img.data, l, r)
    

def EffectFactory_Scroll(img):
  while True:
    for offset in range(img.width + 25):
      
      new_img = Image(img, True)

      for (x, y), v in img.data.iteritems():
        new_img.data[(x - offset, y)] = v

      yield new_img.data
    

factories = []

#factories.append(EffectFactory_Pulse(ImgFile("img/bow.img.raw")))

factories.append(EffectFactory_PanelCycle(ImgFile("img/bow.img.raw")))

#factories.append(Factory_Image(Filter_FakeAlpha(ImgFile("img/WA3.img.raw"))))

factories.append(EffectFactory_Scroll(Filter_Offset(Filter_FakeAlpha(ImgFile("img/want_it_text-57x12-.img.raw")), (-25, 0))))

while 1:
  d = {}
  for factory in factories:
    d.update(factory.next())

  display()

  time.sleep(0.1)



import sys

if len(sys.argv) > 1:
  filepaths = sys.argv[1:]
else:
  filepaths = ["img/WA.img2.raw"]

#imgs = []

d = {}

for filepath in filepaths:
  #imgs.append(readImgFile(filepath, True))
  readImgFile(filepath, True)

display()

#while 1:
#  for d in imgs:
#    display()
#    time.sleep(0.1)



raise SystemExit

r = []

while 1:
  #x = sock.sendto('\x01' + "".join([(chr(random.randint(0, max(25, min(i,255)))) * 3) for i in range(300)]), ("192.168.1.99", 58082) )

  display()

  x = random.randint(0,22)
  y = random.randint(0,9)

  #d = {}

  r.insert(0, (x,y))
  if (len(r) > 4):
    try:
      del d[r.pop()]
    except KeyError:
      pass

  #d[(x,y)] = (0xff,0xff,0xff)
  d[(x+1,y)] = (0xff,0xff,0xff)
  d[(x+1,y+1)] = (0xff,0xff,0xff)
  d[(x,y+1)] = (0xff,0xff,0xff)
  d[(x+2,y+1)] = (0xff,0xff,0xff)
  d[(x+1,y+2)] = (0xff,0xff,0xff)

  for k in d.keys():
    d[k] = tuple([int(i * 0.6) for i in d[k]])

    #time.sleep(0.03)
    #display()

  #print len(r)

  time.sleep(0.075)