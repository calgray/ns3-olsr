import csv
import numpy
from matplotlib import pyplot as plt

def getPlotVectors(throughputs, xCount):
	mins = []
	maxs = []
	avgs = []

	for i in range(1, xCount+1):
		candidates = [float(throughputs[0][i]), float(throughputs[1][i]), float(throughputs[2][i])]
		
		minVal = min(candidates)
		maxVal = max(candidates)
		meanVal = numpy.mean(candidates)
		
		mins.append(minVal)
		maxs.append(maxVal)
		avgs.append(meanVal)

	ys = numpy.array(avgs)
	lowDys = ys - numpy.array(mins)
	highDys = numpy.array(maxs) - ys
	return ys, lowDys, highDys

resultsCsv = csv.reader(open("results.csv", 'r'))
resultsTable = [line for line in resultsCsv]

fig = plt.figure()

#PLOT AGAINST FLOWS

flows = fig.add_subplot(2,1,1)

xs = numpy.linspace(5,30,6)

plainThroughputs = resultsTable[4:7]
plainYs, plainLowDys, plainHighDys = getPlotVectors(plainThroughputs, 6)

mod1Throughputs = resultsTable[16:19]
mod1Ys, mod1LowDys, mod1HighDys = getPlotVectors(mod1Throughputs, 6)

mod2Throughputs = resultsTable[28:31]
mod2Ys, mod2LowDys, mod2HighDys = getPlotVectors(mod2Throughputs, 6)

mod3Throughputs = resultsTable[40:43]
mod3Ys, mod3LowDys, mod3HighDys = getPlotVectors(mod3Throughputs, 6)

mod4Throughputs = resultsTable[52:55]
mod4Ys, mod4LowDys, mod4HighDys = getPlotVectors(mod4Throughputs, 6)

flows.errorbar(xs, plainYs, yerr=[plainLowDys, plainHighDys], label="Plain OLSR")
flows.errorbar(xs, mod1Ys, yerr=[mod1LowDys, mod1HighDys], label="Modification 1")
flows.errorbar(xs, mod2Ys, yerr=[mod2LowDys, mod2HighDys], label="Modification 2")
flows.errorbar(xs, mod3Ys, yerr=[mod3LowDys, mod3HighDys], label="Modification 3")
flows.errorbar(xs, mod4Ys, yerr=[mod4LowDys, mod4HighDys], label="Modification 4")
flows.legend(loc=0)
flows.set_title("Throughput vs flows (20m/s)")
flows.set_xlabel("Number of CBR data flows")
flows.set_ylabel("Total network throughput (kbps)")
flows.set_xlim(0,31)
flows.set_ylim(0)

#PLOT AGAINST MOBILITY

mobility = fig.add_subplot(2,1,2)

xs = numpy.linspace(1,21,5)

plainThroughputs = resultsTable[9:12]
plainYs, plainLowDys, plainHighDys = getPlotVectors(plainThroughputs, 5)

mod1Throughputs = resultsTable[21:24]
mod1Ys, mod1LowDys, mod1HighDys = getPlotVectors(mod1Throughputs, 5)

mod2Throughputs = resultsTable[33:36]
mod2Ys, mod2LowDys, mod2HighDys = getPlotVectors(mod2Throughputs, 5)

mod3Throughputs = resultsTable[45:48]
mod3Ys, mod3LowDys, mod3HighDys = getPlotVectors(mod3Throughputs, 5)

mod4Throughputs = resultsTable[57:60]
mod4Ys, mod4LowDys, mod4HighDys = getPlotVectors(mod4Throughputs, 5)

mobility.errorbar(xs, plainYs, yerr=[plainLowDys, plainHighDys], label="Plain OLSR")
mobility.errorbar(xs, mod1Ys, yerr=[mod1LowDys, mod1HighDys], label="Modification 1")
mobility.errorbar(xs, mod2Ys, yerr=[mod2LowDys, mod2HighDys], label="Modification 2")
mobility.errorbar(xs, mod3Ys, yerr=[mod3LowDys, mod3HighDys], label="Modification 3")
mobility.errorbar(xs, mod4Ys, yerr=[mod4LowDys, mod4HighDys], label="Modification 4")
mobility.legend(loc=0)
mobility.set_title("Throughput vs mobility (20 CBR flows)")
mobility.set_xlabel("Mobility (m/s)")
mobility.set_ylabel("Total network throughput (kbps)")
mobility.set_xlim(0,22)
mobility.set_ylim(0)

plt.tight_layout()
plt.show()