import csv
import numpy
from matplotlib import pyplot as plt

resultsCsv = csv.reader(open("results.csv", 'r'))
resultsTable = [line for line in resultsCsv]

fig = plt.figure()

#PLOT AGAINST FLOWS

flows = fig.add_subplot(2,1,1)

xs = numpy.linspace(5,30,6)

plainYs = [float(y) for y in resultsTable[4][1:7]]
mod1Ys = [float(y) for y in resultsTable[10][1:7]]
mod2Ys = [float(y) for y in resultsTable[16][1:7]]
mod3Ys = [float(y) for y in resultsTable[22][1:7]]
mod4Ys = [float(y) for y in resultsTable[28][1:7]]

flows.plot(xs, plainYs, label="Plain OLSR")
flows.plot(xs, mod1Ys, label="Modification 1")
flows.plot(xs, mod2Ys, label="Modification 2")
flows.plot(xs, mod3Ys, label="Modification 3")
flows.plot(xs, mod4Ys, label="Modification 4")
flows.legend(loc=0)
flows.set_title("Throughput vs flows (20m/s)")
flows.set_xlabel("Number of CBR data flows")
flows.set_ylabel("Total network throughput (kbps)")
flows.set_xlim(0,31)
flows.set_ylim(0)

#PLOT AGAINST MOBILITY

mobility = fig.add_subplot(2,1,2)

xs = numpy.linspace(1,21,5)

plainYs = [float(y) for y in resultsTable[6][1:6]]
mod1Ys = [float(y) for y in resultsTable[12][1:6]]
mod2Ys = [float(y) for y in resultsTable[18][1:6]]
mod3Ys = [float(y) for y in resultsTable[24][1:6]]
mod4Ys = [float(y) for y in resultsTable[30][1:6]]

mobility.plot(xs, plainYs, label="Plain OLSR")
mobility.plot(xs, mod1Ys, label="Modification 1")
mobility.plot(xs, mod2Ys, label="Modification 2")
mobility.plot(xs, mod3Ys, label="Modification 3")
mobility.plot(xs, mod4Ys, label="Modification 4")
mobility.legend(loc=0)
mobility.set_title("Throughput vs mobility (20 CBR flows)")
mobility.set_xlabel("Mobility (m/s)")
mobility.set_ylabel("Total network throughput (kbps)")
mobility.set_xlim(0,22)
mobility.set_ylim(0)

plt.tight_layout()
plt.show()