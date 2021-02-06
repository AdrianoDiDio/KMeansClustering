import matplotlib.pyplot as pyplot
import numpy as np
from numpy.random import Generator,PCG64
import itertools
import csv
import argparse
import os
from pathlib import Path

def checkAndSetArgument(argObject,defaultValue):
    if argObject is not None:
        return argObject
    else:
        return defaultValue
parser = argparse.ArgumentParser(description='Generate an uniform distribution of 2D points.')
parser.add_argument("--width","-w",type=int,help="sets the upper bound of the X axis.")
parser.add_argument("--height","-he",type=int,help="sets the upper bound of the Y axis.")
parser.add_argument("--showPlot","-s",action="store_true",help="show generated points in a plot.")
parser.add_argument("--outFile","-o",type=str,help="set the name of the output file (default is data_blob.csv).")

args = parser.parse_args()

width = checkAndSetArgument(args.width,128)
height = checkAndSetArgument(args.height,64)

print("Selected Upper X Bound: " + str(width))
print("Selected Upper Y Bound: " + str(height))

N = width * height // 4

print("Generating " + str(N) + " samples")

rg = Generator(PCG64())

x = rg.uniform(0,width,N)
roundX = [round(number, 3) for number in x]
y = rg.uniform(0,height,N)
roundY = [round(number, 3) for number in y]

pointList = tuple(map(list, zip(roundX, roundY)))

uniquePointList = np.unique(pointList, axis=0)

uniqueX,uniqueY = zip(*uniquePointList)

if args.showPlot is not False:
    pyplot.scatter(uniqueX, uniqueY,c='g', alpha=0.6, lw=0)
    pyplot.show()
    
csvOutFile = checkAndSetArgument(args.outFile,'data_blob.csv')
csvOutFilePath = os.path.dirname(os.path.realpath(csvOutFile))
Path(csvOutFilePath).mkdir(parents=True, exist_ok=True)
with open(csvOutFile, newline='',mode='w') as dataBlobFile:
    dataBlobWriter = csv.writer(dataBlobFile,delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
    dataBlobWriter.writerow(['x','y'])
    for row in uniquePointList:
        dataBlobWriter.writerow(row)
